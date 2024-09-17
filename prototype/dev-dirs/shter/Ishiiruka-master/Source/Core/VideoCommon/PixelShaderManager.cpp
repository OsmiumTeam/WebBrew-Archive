// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cmath>
#include <cstring>

#include "Common/ChunkFile.h"
#include "Common/CommonTypes.h"

#include "VideoCommon/PixelShaderManager.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/XFMemory.h"
#include "VideoCommon/ConstantManager.h"


alignas(256) float PixelShaderManager::psconstants[PixelShaderManager::ConstantBufferSize];
PixelShaderConstants* PixelShaderManager::Debug_view = (PixelShaderConstants*)&PixelShaderManager::psconstants[0];
ConstatBuffer PixelShaderManager::m_buffer(PixelShaderManager::psconstants, PixelShaderManager::ConstantBufferSize);
static int s_nColorsChanged[2]; // 0 - regular colors, 1 - k colors
int PixelShaderManager::s_nIndTexMtxChanged;
bool PixelShaderManager::s_bAlphaChanged;
bool PixelShaderManager::s_bZBiasChanged;
bool PixelShaderManager::s_bZTextureTypeChanged;
bool PixelShaderManager::s_bFogColorChanged;
bool PixelShaderManager::s_bFogParamChanged;
bool PixelShaderManager::s_bFogRangeAdjustChanged;
bool PixelShaderManager::s_bViewPortChanged;
bool PixelShaderManager::s_EfbScaleChanged;
bool PixelShaderManager::s_bIndirectDirty;
bool PixelShaderManager::s_bDestAlphaDirty;

static int nLightsChanged[2]; // min,max
static int lastRGBAfull[2][4][4];
u8 PixelShaderManager::s_nTexDimsChanged;
u8 PixelShaderManager::s_nTexLayersChanged;
u8 PixelShaderManager::s_nIndTexScaleChanged;
static u32 lastAlpha;
static u32 lastTexDims[8]; // width | height << 16 | wrap_s << 28 | wrap_t << 30
static uint4 lastTexLayers[8];
static u32 lastZBias;
int PixelShaderManager::s_materials_changed;
static int sflags[4];
static bool sbflagschanged;
static int s_LightsPhong[8];

const float U8_NORM_COEF = 1.0f / 255.0f;
const float U10_NORM_COEF = 1.0f / 1023.0f;
const float U24_NORM_COEF = 1 / 16777216.0f;
static bool s_use_integer_constants = false;


void PixelShaderManager::Init(bool use_integer_constants)
{
  s_use_integer_constants = use_integer_constants;
  m_buffer.Clear();
  lastAlpha = 0;
  memset(lastTexDims, 0, sizeof(lastTexDims));
  lastZBias = 0;
  memset(lastRGBAfull, 0, sizeof(lastRGBAfull));
  memset(sflags, 0, sizeof(sflags));
  memset(s_LightsPhong, 0, sizeof(s_LightsPhong));

  m_buffer.SetConstant4<u32>(C_UBERKONST, 255, 255, 255, 255);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 1, 223, 223, 223, 223);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 2, 191, 191, 191, 191);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 3, 159, 159, 159, 159);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 4, 128, 128, 128, 128);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 5, 96, 96, 96, 96);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 6, 64, 64, 64, 64);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 7, 32, 32, 32, 32);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 8, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 9, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 10, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 11, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 12, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 13, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 14, 0, 0, 0, 0);
  m_buffer.SetConstant4<u32>(C_UBERKONST + 15, 0, 0, 0, 0);

  Dirty();
}

void PixelShaderManager::Dirty()
{
  s_nColorsChanged[0] = s_nColorsChanged[1] = 15;
  s_nTexDimsChanged = 0xFF;
  s_nIndTexScaleChanged = 0xFF;
  s_nIndTexMtxChanged = 15;
  s_bAlphaChanged = s_bZBiasChanged = s_bZTextureTypeChanged = s_bViewPortChanged = true;
  s_bFogRangeAdjustChanged = s_bFogColorChanged = s_bFogParamChanged = true;
  nLightsChanged[0] = 0; nLightsChanged[1] = 0x80;
  s_materials_changed = 15;
  sbflagschanged = true;
  s_EfbScaleChanged = true;
  s_bIndirectDirty = true;
  s_bDestAlphaDirty = true;
}

const float* PixelShaderManager::GetBuffer()
{
  return psconstants;
}

float* PixelShaderManager::GetBufferToUpdate(u32 const_number, u32 size)
{
  return m_buffer.GetBufferToUpdate<float>(const_number, size);
}

const regionvector &PixelShaderManager::GetDirtyRegions()
{
  return m_buffer.GetRegions();
}

void PixelShaderManager::EnableDirtyRegions()
{
  m_buffer.EnableDirtyRegions();
}
void PixelShaderManager::DisableDirtyRegions()
{
  m_buffer.DisableDirtyRegions();
}

void PixelShaderManager::SetConstants()
{
  if (s_EfbScaleChanged)
  {
    m_buffer.SetConstant4<float>(C_EFBSCALE,
      1.0f / float(g_renderer->EFBToScaledXf(1)),
      1.0f / float(g_renderer->EFBToScaledYf(1)), 0.0f, 0.0f);
    s_EfbScaleChanged = true;
    s_bViewPortChanged = true;
  }

  for (int i = 0; i < 2; ++i)
  {
    if (s_nColorsChanged[i])
    {
      int baseind = i ? C_KCOLORS : C_COLORS;
      for (int j = 0; j < 4; ++j)
      {
        if ((s_nColorsChanged[i] & (1 << j)))
        {
          int* src = &lastRGBAfull[i][j][0];
          if (s_use_integer_constants)
            m_buffer.SetConstant4v<int>(baseind + j, src);
          else
            m_buffer.SetConstant4<float>(baseind + j, (float)src[0], (float)src[1], (float)src[2], (float)src[3]);
          s_nColorsChanged[i] &= ~(1 << j);
        }
      }
    }
  }

  if (s_nTexDimsChanged)
  {
    for (int i = 0; i < 8; ++i)
    {
      if ((s_nTexDimsChanged & (1 << i)))
      {
        SetPSTextureDims(i);
        s_nTexDimsChanged &= ~(1 << i);
      }
    }
  }

  if (s_nTexLayersChanged)
  {
    for (int i = 0; i < 8; ++i)
    {
      if ((s_nTexLayersChanged & (1 << i)))
      {
       m_buffer.SetConstant4<float>(C_TEXLAYERS + i, float(lastTexLayers[i][0]), float(lastTexLayers[i][1]), float(lastTexLayers[i][2]), 0.0f);
        s_nTexLayersChanged &= ~(1 << i);
      }
    }
  }

  if (s_bAlphaChanged)
  {
    if (s_use_integer_constants)
      m_buffer.SetConstant4<int>(C_ALPHA, lastAlpha & 0xff, (lastAlpha >> 8) & 0xff, 0, (lastAlpha >> 16) & 0xff);
    else
      m_buffer.SetConstant4<float>(C_ALPHA, ((float)(lastAlpha & 0xff)), ((float)((lastAlpha >> 8) & 0xff)), 0.0f, ((float)((lastAlpha >> 16) & 0xff)));
    s_bAlphaChanged = false;
  }

  if (s_bZTextureTypeChanged)
  {
    if (s_use_integer_constants)
    {
      int *temp = m_buffer.GetBufferToUpdate<int>(C_ZBIAS, 1);
      switch (bpmem.ztex2.type.Value())
      {
      case TEV_ZTEX_TYPE_U8:
        // 8 bits
        temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 1;
        break;
      case TEV_ZTEX_TYPE_U16:
        // 16 bits
        temp[0] = 1; temp[1] = 0; temp[2] = 0; temp[3] = 256;
        break;
      case TEV_ZTEX_TYPE_U24:
        // 24 bits
        temp[0] = 65536; temp[1] = 256; temp[2] = 1; temp[3] = 0;
        break;
      }
    }
    else
    {
      float *ftemp = m_buffer.GetBufferToUpdate<float>(C_ZBIAS, 1);
      switch (bpmem.ztex2.type.Value())
      {
      case TEV_ZTEX_TYPE_U8:
        // 8 bits
        ftemp[0] = 0; ftemp[1] = 0; ftemp[2] = 0; ftemp[3] = 255.0f / 16777215.0f;
        break;
      case TEV_ZTEX_TYPE_U16:
        // 16 bits
        ftemp[0] = 255.0f / 16777215.0f; ftemp[1] = 0; ftemp[2] = 0; ftemp[3] = 65280.0f / 16777215.0f;
        break;
      case TEV_ZTEX_TYPE_U24:
        // 24 bits
        ftemp[0] = 16711680.0f / 16777215.0f; ftemp[1] = 65280.0f / 16777215.0f; ftemp[2] = 255.0f / 16777215.0f; ftemp[3] = 0;
        break;
      }
    }
    s_bZTextureTypeChanged = false;
  }

  if (s_bZBiasChanged || s_bViewPortChanged)
  {
    // reversed gxsetviewport(xorig, yorig, width, height, nearz, farz)
    // [0] = width/2
    // [1] = height/2
    // [2] = 16777215 * (farz - nearz)
    // [3] = xorig + width/2 + 342
    // [4] = yorig + height/2 + 342
    // [5] = 16777215 * farz

    //ERROR_LOG("pixel=%x,%x, bias=%x\n", bpmem.zcontrol.pixel_format, bpmem.ztex2.type, lastZBias);
    if (s_use_integer_constants)
    {
      m_buffer.SetConstant4<int>(C_ZBIAS + 1,
        (int)xfmem.viewport.farZ,
        (int)xfmem.viewport.zRange,
        0,
        lastZBias);
    }
    else
    {
      m_buffer.SetConstant4<float>(C_ZBIAS + 1,
        xfmem.viewport.farZ,
        xfmem.viewport.zRange,
        0.0f,
        (float)(lastZBias) / 16777215.0f);
    }
    s_bZBiasChanged = false;
  }

  // indirect incoming texture scales
  if (s_nIndTexScaleChanged)
  {
    // set as two sets of vec4s, each containing S and T of two ind stages.
    if (s_nIndTexScaleChanged & 0x03)
    {
      if (s_use_integer_constants)
      {
        int *i = m_buffer.GetBufferToUpdate<int>(C_INDTEXSCALE, 1);
        i[0] = bpmem.texscale[0].ss0;
        i[1] = bpmem.texscale[0].ts0;
        i[2] = bpmem.texscale[0].ss1;
        i[3] = bpmem.texscale[0].ts1;
      }
      else
      {
        float *f = m_buffer.GetBufferToUpdate<float>(C_INDTEXSCALE, 1);
        f[0] = (float)bpmem.texscale[0].ss0;
        f[1] = (float)bpmem.texscale[0].ts0;
        f[2] = (float)bpmem.texscale[0].ss1;
        f[3] = (float)bpmem.texscale[0].ts1;
      }
    }

    if (s_nIndTexScaleChanged & 0x0c)
    {
      if (s_use_integer_constants)
      {
        int *i = m_buffer.GetBufferToUpdate<int>(C_INDTEXSCALE + 1, 1);
        i[0] = bpmem.texscale[1].ss0;
        i[1] = bpmem.texscale[1].ts0;
        i[2] = bpmem.texscale[1].ss1;
        i[3] = bpmem.texscale[1].ts1;
      }
      else
      {
        float *f = m_buffer.GetBufferToUpdate<float>(C_INDTEXSCALE + 1, 1);
        f[0] = (float)bpmem.texscale[1].ss0;
        f[1] = (float)bpmem.texscale[1].ts0;
        f[2] = (float)bpmem.texscale[1].ss1;
        f[3] = (float)bpmem.texscale[1].ts1;
      }
    }
    s_nIndTexScaleChanged = 0;
  }

  if (s_nIndTexMtxChanged)
  {
    for (int i = 0; i < 3; ++i)
    {
      if (s_nIndTexMtxChanged & (1 << i))
      {
        int scale = ((u32)bpmem.indmtx[i].col0.s0 << 0) |
          ((u32)bpmem.indmtx[i].col1.s1 << 2) |
          ((u32)bpmem.indmtx[i].col2.s2 << 4);
        scale = 17 - scale;
        if (s_use_integer_constants)
        {
          m_buffer.SetConstant4<int>(C_INDTEXMTX + 2 * i,
            bpmem.indmtx[i].col0.ma,
            bpmem.indmtx[i].col1.mc,
            bpmem.indmtx[i].col2.me,
            scale);
          m_buffer.SetConstant4<int>(C_INDTEXMTX + 2 * i + 1,
            bpmem.indmtx[i].col0.mb,
            bpmem.indmtx[i].col1.md,
            bpmem.indmtx[i].col2.mf,
            scale);
        }
        else
        {
          m_buffer.SetConstant4<float>(C_INDTEXMTX + 2 * i,
            (float)bpmem.indmtx[i].col0.ma,
            (float)bpmem.indmtx[i].col1.mc,
            (float)bpmem.indmtx[i].col2.me,
            (float)scale);
          m_buffer.SetConstant4<float>(C_INDTEXMTX + 2 * i + 1,
            (float)bpmem.indmtx[i].col0.mb,
            (float)bpmem.indmtx[i].col1.md,
            (float)bpmem.indmtx[i].col2.mf,
            (float)scale);
        }
        PRIM_LOG("indmtx%d: scale=%i, mat=(%i %i %i; %i %i %i)\n",
          i, scale,
          bpmem.indmtx[i].col0.ma, bpmem.indmtx[i].col1.mc, bpmem.indmtx[i].col2.me,
          bpmem.indmtx[i].col0.mb, bpmem.indmtx[i].col1.md, bpmem.indmtx[i].col2.mf);

        s_nIndTexMtxChanged &= ~(1 << i);
      }
    }
  }

  if (s_bFogColorChanged)
  {
    if (s_use_integer_constants)
      m_buffer.SetConstant4<int>(C_FOGCOLOR, bpmem.fog.color.r.Value(), bpmem.fog.color.g.Value(), bpmem.fog.color.b.Value(), 0);
    else
      m_buffer.SetConstant4<float>(C_FOGCOLOR, (float)bpmem.fog.color.r.Value(), (float)bpmem.fog.color.g.Value(), (float)bpmem.fog.color.b.Value(), 0.0f);
    s_bFogColorChanged = false;
  }

  if (s_bFogParamChanged)
  {
    if (!g_ActiveConfig.bDisableFog)
    {
      if (s_use_integer_constants)
      {
        m_buffer.SetConstant4<int>(C_FOGI, 0, bpmem.fog.b_magnitude, 0, bpmem.fog.b_shift);
      }
      else
      {
        //downscale magnitude to 0.24 bits
        float b = (float)bpmem.fog.b_magnitude / float(0xFFFFFF);
        float b_shf = (1.0f / (float)(1 << bpmem.fog.b_shift));
        m_buffer.SetConstant4<float>(C_FOGI, 0.0f, b, 0.0f, b_shf);
      }
      m_buffer.SetConstant4<float>(C_FOGF + 1, bpmem.fog.a.GetA(), 0, bpmem.fog.c_proj_fsel.GetC(), 0);
      m_buffer.SetConstant(C_UBERPARAM0, 2, bpmem.fog.c_proj_fsel.hex);
    }
    else
    {
      m_buffer.SetConstant4<float>(C_FOGF + 1, 0.0f, 1.0f, 0.0f, 1.0f);
      m_buffer.SetConstant4<float>(C_FOGI, 0.0f, 1.0f, 0.0f, 1.0f);
      if ((g_ActiveConfig.backend_info.APIType & API_D3D9) == 0)
      {
        m_buffer.SetConstant(C_UBERPARAM0, 2, 0);
      }
    }
    s_bFogParamChanged = false;
  }

  if (s_bFogRangeAdjustChanged || s_bViewPortChanged)
  {
    if (!g_ActiveConfig.bDisableFog && bpmem.fogRange.Base.Enabled.Value() == 1)
    {
      //bpmem.fogRange.Base.Center : center of the viewport in x axis. observation: bpmem.fogRange.Base.Center = realcenter + 342;
      int center = ((u32)bpmem.fogRange.Base.Center.Value()) - 342;
      // normalize center to make calculations easy
      float ScreenSpaceCenter = center / (2.0f * xfmem.viewport.wd);
      ScreenSpaceCenter = (ScreenSpaceCenter * 2.0f) - 1.0f;
      //bpmem.fogRange.K seems to be  a table of precalculated coefficients for the adjust factor
      //observations: bpmem.fogRange.K[0].LO appears to be the lowest value and bpmem.fogRange.K[4].HI the largest
      // they always seems to be larger than 256 so my theory is :
      // they are the coefficients from the center to the border of the screen
      // so to simplify I use the hi coefficient as K in the shader taking 256 as the scale
      m_buffer.SetConstant4<float>(C_FOGF
        , ScreenSpaceCenter
        , (float)g_renderer->EFBToScaledX((int)(2.0f * xfmem.viewport.wd))
        , bpmem.fogRange.K[4].HI.Value() / 256.0f
        , 0.0f);
    }
    else
    {
      m_buffer.SetConstant4<float>(C_FOGF, 0.0f, 1.0f, 1.0f, 0.0f); // Need to update these values for older hardware that fails to divide by zero in shaders.
    }
    if (m_buffer.GetBuffer<u32>(C_UBERPARAM0)[3] != bpmem.fogRange.Base.hex && (g_ActiveConfig.backend_info.APIType & API_D3D9) == 0)
    {
      m_buffer.SetConstant(C_UBERPARAM0, 3, bpmem.fogRange.Base.hex);
    }
    s_bFogRangeAdjustChanged = false;
  }
  if (((g_ActiveConfig.backend_info.APIType & API_D3D9)) != 0 && g_ActiveConfig.PixelLightingEnabled(xfmem, VertexLoaderManager::g_current_components))
  {
    if (nLightsChanged[0] >= 0)
    {
      // lights don't have a 1 to 1 mapping, the color component needs to be converted to 4 floats
      int istart = nLightsChanged[0] / 0x10;
      int iend = (nLightsChanged[1] + 15) / 0x10;

      for (int i = istart; i < iend; ++i)
      {
        const Light& light = xfmem.lights[i];

        m_buffer.SetConstant4<float>(C_PLIGHTS + 5 * i,
          float(light.color[3]),
          float(light.color[2]),
          float(light.color[1]),
          float(light.color[0]));
        m_buffer.SetConstant3v(C_PLIGHTS + 5 * i + 1, light.cosatt);
        if (fabs(light.distatt[0]) < 0.00001f &&
          fabs(light.distatt[1]) < 0.00001f &&
          fabs(light.distatt[2]) < 0.00001f)
        {
          // dist attenuation, make sure not equal to 0!!!
          m_buffer.SetConstant4(C_PLIGHTS + 5 * i + 2, 0.00001f, light.distatt[1], light.distatt[2], 0.0f);
        }
        else
        {
          m_buffer.SetConstant3v(C_PLIGHTS + 5 * i + 2, light.distatt);
        }
        m_buffer.SetConstant3v(C_PLIGHTS + 5 * i + 3, light.dpos);
        double norm = double(light.ddir[0]) * double(light.ddir[0]) +
          double(light.ddir[1]) * double(light.ddir[1]) +
          double(light.ddir[2]) * double(light.ddir[2]);
        norm = 1.0 / sqrt(norm);
        float norm_float = static_cast<float>(norm);
        m_buffer.SetConstant4(C_PLIGHTS + 5 * i + 4, light.ddir[0] * norm_float, light.ddir[1] * norm_float, light.ddir[2] * norm_float, 0.0f);
      }

      nLightsChanged[0] = nLightsChanged[1] = -1;
    }

    if (s_materials_changed)
    {
      for (int i = 0; i < 2; ++i)
      {
        if (s_materials_changed & (1 << i))
        {
          u32 data = xfmem.ambColor[i];
          m_buffer.SetConstant4<float>(C_PMATERIALS + i,
            float((data >> 24) & 0xFF),
            float((data >> 16) & 0xFF),
            float((data >> 8) & 0xFF),
            float(data & 0xFF));
        }
      }

      for (int i = 0; i < 2; ++i)
      {
        if (s_materials_changed & (1 << (i + 2)))
        {
          u32 data = xfmem.matColor[i];
          m_buffer.SetConstant4<float>(C_PMATERIALS + i + 2,
            float((data >> 24) & 0xFF),
            float((data >> 16) & 0xFF),
            float((data >> 8) & 0xFF),
            float(data & 0xFF));
        }
      }
      s_materials_changed = 0;
    }
  }
  if (g_ActiveConfig.iRimBase != s_LightsPhong[0]
    || g_ActiveConfig.iRimPower != s_LightsPhong[1]
    || g_ActiveConfig.iRimIntesity != s_LightsPhong[2]
    || g_ActiveConfig.iSpecularMultiplier != s_LightsPhong[3])
  {
    s_LightsPhong[0] = g_ActiveConfig.iRimBase;
    s_LightsPhong[1] = g_ActiveConfig.iRimPower;
    s_LightsPhong[2] = g_ActiveConfig.iRimIntesity;
    s_LightsPhong[3] = g_ActiveConfig.iSpecularMultiplier;
    m_buffer.SetConstant4(C_PPHONG
      , float(g_ActiveConfig.iRimBase)
      , 1.0f + U8_NORM_COEF * g_ActiveConfig.iRimPower * 2.0f
      , U8_NORM_COEF * g_ActiveConfig.iRimIntesity
      , U8_NORM_COEF * g_ActiveConfig.iSpecularMultiplier);
  }
  if (g_ActiveConfig.iSimBumpStrength != s_LightsPhong[4]
    || g_ActiveConfig.iSimBumpThreshold != s_LightsPhong[5]
    || g_ActiveConfig.iSimBumpDetailBlend != s_LightsPhong[6]
    || g_ActiveConfig.iSimBumpDetailFrequency != s_LightsPhong[7])
  {
    s_LightsPhong[4] = g_ActiveConfig.iSimBumpStrength;
    s_LightsPhong[5] = g_ActiveConfig.iSimBumpThreshold;
    s_LightsPhong[6] = g_ActiveConfig.iSimBumpDetailBlend;
    s_LightsPhong[7] = g_ActiveConfig.iSimBumpDetailFrequency;
    float bump_strenght = U10_NORM_COEF * g_ActiveConfig.iSimBumpStrength;
    m_buffer.SetConstant4(C_PPHONG + 1
      , bump_strenght * bump_strenght
      , U8_NORM_COEF * g_ActiveConfig.iSimBumpThreshold * 16.0f
      , U8_NORM_COEF * g_ActiveConfig.iSimBumpDetailBlend
      , float(g_ActiveConfig.iSimBumpDetailFrequency));
  }
  if (sbflagschanged)
  {
    sbflagschanged = false;
    m_buffer.SetConstant4v<int>(C_FLAGS, sflags);
  }
  if (s_bIndirectDirty && (g_ActiveConfig.backend_info.APIType & API_D3D9) == 0)
  {
    for (int i = 0; i < 4; i++)
      m_buffer.SetConstant(C_UBERPACK1 + i, 3, 0u);

    for (u32 i = 0; i < (bpmem.genMode.numtevstages.Value() + 1); ++i)
    {
      u32 stage = bpmem.tevind[i].bt.Value();
      if (stage < bpmem.genMode.numindstages)
      {
        // We set some extra bits so the ubershader can quickly check if these
        // features are in use.
        if (bpmem.tevind[i].IsActive())
          m_buffer.SetConstant(C_UBERPACK1 + stage, 3,
            bpmem.tevindref.getTexCoord(stage) | bpmem.tevindref.getTexMap(stage) << 8 | 1 << 16);
        // Note: a tevind of zero just happens to be a passthrough, so no need
        // to set an extra bit.
        m_buffer.SetConstant(C_UBERPACK1 + i, 2,
          bpmem.tevind[i].hex | 0x80000000u);
        // TODO: This match shadergen, but videosw will
        // always wrap.

        // The ubershader uses tevind != 0 as a condition whether to calculate texcoords,
        // even when texture is disabled, instead of the stage < bpmem.genMode.numindstages.
        // We set an unused bit here to indicate that the stage is active, even if it
        // is just a pass-through.        
      }
      else
      {
        m_buffer.SetConstant(C_UBERPACK1 + i, 2, 0);
      }
    }
    s_bIndirectDirty = false;
  }

  if (s_bDestAlphaDirty && (g_ActiveConfig.backend_info.APIType & API_D3D9) == 0)
  {
    // Destination alpha is only enabled if alpha writes are enabled. Force entire uniform to zero
    // when disabled.
    u32 dstalpha = bpmem.blendmode.alphaupdate.Value() != 0 && bpmem.dstalpha.enable.Value() != 0 &&
      bpmem.zcontrol.pixel_format.Value() == PEControl::RGBA6_Z24 ?
      bpmem.dstalpha.hex :
      0;

    if (m_buffer.GetBuffer<u32>(C_UBERPARAM1)[0] != dstalpha)
    {
      m_buffer.SetConstant(C_UBERPARAM1, 0, dstalpha);
    }
  }
}

void PixelShaderManager::SetPSTextureDims(int texid)
{
  // texdims.xy are reciprocals of the real texture dimensions
  // texdims.zw are the scaled dimensions
  float* fdims = m_buffer.GetBufferToUpdate<float>(C_TEXDIMS + texid, 1);
  TCoordInfo& tc = bpmem.texcoords[texid];
  fdims[0] = 1.0f / float((lastTexDims[texid] & 0xffff) << 7);
  fdims[1] = 1.0f / float(((lastTexDims[texid] >> 16) & 0xffff) << 7);
  fdims[2] = float((tc.s.scale_minus_1 + 1) << 7);
  fdims[3] = float((tc.t.scale_minus_1 + 1) << 7);
}

void PixelShaderManager::SetTevColor(int index, int component, s32 value)
{
  s32 *c = &lastRGBAfull[0][index][0];
  if (c[component] != value)
  {
    c[component] = value;
    s_nColorsChanged[0] |= 1 << index;
  }
}

void PixelShaderManager::SetTevKonstColor(int index, int component, s32 value)
{
  s32 *c = &lastRGBAfull[1][index][0];
  if (c[component] != value)
  {
    c[component] = value;
    s_nColorsChanged[1] |= 1 << index;
    if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
    {
      return;
    }
    // Konst for ubershaders. We build the whole array on cpu so the gpu can do a single indirect
    // access.
    if (component != 3)  // Alpha doesn't included in the .rgb konsts
      m_buffer.SetConstant(C_UBERKONST + index + 12, component, value);

    // .rrrr .gggg .bbbb .aaaa konsts
    m_buffer.SetConstant4(C_UBERKONST + index + 16 + component * 4, value, value, value, value);
  }
}

void PixelShaderManager::SetTevOrder(int index, u32 order)
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  if (m_buffer.GetBuffer<u32>(C_UBERPACK2 + index)[0] != order)
  {
    m_buffer.SetConstant(C_UBERPACK2 + index, 0, order);
  }
}

void PixelShaderManager::SetTevKSel(int index, u32 ksel)
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  if (m_buffer.GetBuffer<u32>(C_UBERPACK2 + index)[1] != ksel)
  {
    m_buffer.SetConstant(C_UBERPACK2 + index, 1, ksel);
  }
}

void PixelShaderManager::SetTevCombiner(int index, int alpha, u32 combiner)
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  if (m_buffer.GetBuffer<u32>(C_UBERPACK1 + index)[alpha] != combiner)
  {
    m_buffer.SetConstant(C_UBERPACK1 + index, alpha, combiner);
  }
}

void PixelShaderManager::SetTevIndirectChanged()
{
  s_bIndirectDirty = true;
}

void PixelShaderManager::SetAlpha()
{
  if ((bpmem.alpha_test.hex & 0xffff) != lastAlpha)
  {
    lastAlpha = (lastAlpha & ~0xffff) | (bpmem.alpha_test.hex & 0xffff);
    s_bAlphaChanged = true;
  }
}

void PixelShaderManager::SetAlphaTestChanged()
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  // Force alphaTest Uniform to zero if it will always pass.
  // (set an extra bit to distinguish from "never && never")
  // TODO: we could optimize this further and check the actual constants,
  // i.e. "a <= 0" and "a >= 255" will always pass.
  u32 alpha_test =
    bpmem.alpha_test.TestResult() != AlphaTest::PASS ? bpmem.alpha_test.hex | 1 << 31 : 0;
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM0)[1] != alpha_test)
  {
    m_buffer.SetConstant(C_UBERPARAM0, 1, alpha_test);
  }
}

void PixelShaderManager::SetDestAlphaChanged()
{
  if (bpmem.dstalpha.alpha.Value() != (lastAlpha >> 16))
  {
    lastAlpha = (lastAlpha & ~0xff0000) | ((bpmem.dstalpha.alpha.Value() & 0xff) << 16);
    s_bAlphaChanged = true;
  }
}

void PixelShaderManager::SetTexDims(int texmapid, u32 width, u32 height, u32 layers, u32 normallayer, u32 emissivelayer)
{
  u32 wh = width | (height << 16);
  if (lastTexDims[texmapid] != wh)
  {
    lastTexDims[texmapid] = wh;
    s_nTexDimsChanged |= 1 << texmapid;
  }
  if (lastTexLayers[texmapid][0] != layers)
  {
    lastTexLayers[texmapid][0] = layers;
    s_nTexLayersChanged |= 1 << texmapid;
  }
  if (lastTexLayers[texmapid][1] != normallayer)
  {
    lastTexLayers[texmapid][1] = normallayer;
    s_nTexLayersChanged |= 1 << texmapid;
  }
  if (lastTexLayers[texmapid][2] != emissivelayer)
  {
    lastTexLayers[texmapid][2] = emissivelayer;
    s_nTexLayersChanged |= 1 << texmapid;
  }
}

void PixelShaderManager::SetZTextureBias()
{
  if (lastZBias != bpmem.ztex1.bias.Value())
  {
    s_bZBiasChanged = true;
    lastZBias = bpmem.ztex1.bias.Value();
  }
}

void PixelShaderManager::SetZTextureOpChanged()
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM1)[1] != bpmem.ztex2.op.Value())
  {
    m_buffer.SetConstant(C_UBERPARAM1, 1, bpmem.ztex2.op.Value());
  }
}

void PixelShaderManager::SetColorMatrix(const float* pmatrix)
{
  m_buffer.SetMultiConstant4v<float>(C_COLORMATRIX, 7, pmatrix);
  s_nColorsChanged[0] = s_nColorsChanged[1] = 15;
}

void PixelShaderManager::InvalidateXFRange(u32 start, u32 end)
{
  if (start < XFMEM_LIGHTS_END && end > XFMEM_LIGHTS)
  {
    int _start = start < XFMEM_LIGHTS ? XFMEM_LIGHTS : start - XFMEM_LIGHTS;
    int _end = end < XFMEM_LIGHTS_END ? end - XFMEM_LIGHTS : XFMEM_LIGHTS_END - XFMEM_LIGHTS;

    if (nLightsChanged[0] == -1)
    {
      nLightsChanged[0] = _start;
      nLightsChanged[1] = _end;
    }
    else
    {
      if (nLightsChanged[0] > _start) nLightsChanged[0] = _start;
      if (nLightsChanged[1] < _end)   nLightsChanged[1] = _end;
    }
  }
}

void PixelShaderManager::SetZSlope(float dfdx, float dfdy, float f0)
{
  m_buffer.SetConstant4(C_ZSLOPE,
    dfdx,
    dfdy,
    f0,
    0.0f);
}

void PixelShaderManager::SetFlags(int index, int mask, int value)
{
  int newflag = (sflags[index] & (~mask)) | (value & mask);
  if (newflag != sflags[index])
  {
    sbflagschanged = true;
    sflags[index] = newflag;
  }
}

void PixelShaderManager::SetGenModeChanged()
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM0)[0] != bpmem.genMode.hex)
  {
    m_buffer.SetConstant(C_UBERPARAM0, 0, bpmem.genMode.hex);
  }
  s_bIndirectDirty = true;
}

void PixelShaderManager::SetZModeControl()
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  u32 late_ztest = bpmem.UseLateDepthTest();
  u32 rgba6_format =
    (bpmem.zcontrol.pixel_format.Value() == PEControl::RGBA6_Z24 && !g_ActiveConfig.bForceTrueColor) ? 1 :
    0;
  u32 dither = (rgba6_format || g_ActiveConfig.bForcedDithering) 
    && bpmem.blendmode.dither.Value() > 0
    && (bpmem.blendmode.logicopenable.Value() == 0 || bpmem.blendmode.blendenable.Value() > 0 || bpmem.blendmode.subtract);
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM1)[2] != late_ztest
    || m_buffer.GetBuffer<u32>(C_UBERPARAM1)[3] != rgba6_format
    || m_buffer.GetBuffer<u32>(C_UBERPARAM2)[0] != dither)
  {
    m_buffer.SetConstant(C_UBERPARAM1, 2, late_ztest);
    m_buffer.SetConstant(C_UBERPARAM1, 3, rgba6_format);
    m_buffer.SetConstant(C_UBERPARAM2, 0, dither);
  }
  s_bDestAlphaDirty = true;
}

void PixelShaderManager::SetBlendModeChanged()
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  u32 rgba6_format =
    (bpmem.zcontrol.pixel_format.Value() == PEControl::RGBA6_Z24 && !g_ActiveConfig.bForceTrueColor) ? 1 :
    0;
  u32 dither = (rgba6_format || g_ActiveConfig.bForcedDithering)
    && bpmem.blendmode.dither.Value() > 0
    && (bpmem.blendmode.logicopenable.Value() == 0 || bpmem.blendmode.blendenable.Value() > 0 || bpmem.blendmode.subtract);
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM2)[0] != dither)
  {
    m_buffer.SetConstant(C_UBERPARAM2, 0, dither);
  }
  s_bDestAlphaDirty = true;
}

void PixelShaderManager::SetBoundingBoxActive(bool active)
{
  if ((g_ActiveConfig.backend_info.APIType & API_D3D9) != 0)
  {
    return;
  }
  const u32 enable =
    active && g_ActiveConfig.iBBoxMode == BBoxGPU ? 1 : 0;
  if (m_buffer.GetBuffer<u32>(C_UBERPARAM2)[1] != enable)
  {
    m_buffer.SetConstant(C_UBERPARAM2, 1, enable);
  }
}

void PixelShaderManager::DoState(PointerWrap &p)
{
  p.Do(lastRGBAfull);
  p.Do(lastAlpha);
  p.Do(lastTexDims);
  p.Do(lastZBias);

  if (p.GetMode() == PointerWrap::MODE_READ)
  {
    Dirty();
  }
}
