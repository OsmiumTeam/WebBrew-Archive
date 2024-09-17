// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.
#include <memory>

#include "Common/MsgHandler.h"
#include "VideoBackends/D3D12/D3DBase.h"
#include "VideoBackends/D3D12/D3DCommandListManager.h"
#include "VideoBackends/D3D12/D3DDescriptorHeapManager.h"
#include "VideoBackends/D3D12/D3DStreamBuffer.h"
#include "VideoBackends/D3D12/D3DTexture.h"
#include "VideoBackends/D3D12/D3DUtil.h"
#include "VideoBackends/D3D12/FramebufferManager.h"
#include "VideoBackends/D3D12/Render.h"

namespace DX12
{
namespace D3D
{
constexpr size_t SYNC_TEXTURE_UPLOAD_BUFFER_SIZE = 4 * 512 * 512;
constexpr size_t INITIAL_TEXTURE_UPLOAD_BUFFER_SIZE = 8 * 1024 * 1024;
constexpr size_t MAXIMUM_TEXTURE_UPLOAD_BUFFER_SIZE = 64 * 1024 * 1024;

static std::unique_ptr<D3DStreamBuffer> s_texture_upload_stream_buffer;

void CleanupPersistentD3DTextureResources()
{
  s_texture_upload_stream_buffer.reset();
}

void ReplaceTexture2D(ID3D12Resource* texture12, const u8* buffer, DXGI_FORMAT fmt, u32 width,
                      u32 height, u32 src_pitch, u32 level, u32 layer, u32 miplevels, u32 layers,
                      D3D12_RESOURCE_STATES current_resource_state)
{
  u32 pixelsize = 1;
  bool compresed = false;
  switch (fmt)
  {
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R32_FLOAT:
    pixelsize = 4;
    break;
  case DXGI_FORMAT_B5G6R5_UNORM:
    pixelsize = 2;
    break;
  case DXGI_FORMAT_BC1_UNORM:
    pixelsize = 8;
    compresed = true;
    break;
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC7_UNORM:
    pixelsize = 16;
    compresed = true;
    break;
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
    pixelsize = 8;
    break;
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
    pixelsize = 16;
    break;
  default:
    break;
  }
  if (!compresed)
  {
    src_pitch *= pixelsize;
  }
  else
  {
    src_pitch = std::max(1u, (src_pitch + 3) >> 2);
    src_pitch *= pixelsize;
    height = std::max(1u, (height + 3) >> 2);
  }
  const u32 upload_size =
      Common::AlignUpSizePow2(src_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * height;

  ID3D12Resource* upload_buffer = nullptr;
  size_t upload_buffer_offset = 0;
  u8* dest_data = nullptr;
  if (upload_size > MAXIMUM_TEXTURE_UPLOAD_BUFFER_SIZE)
  {
    // If the texture is too large to fit in the upload buffer, create a temporary buffer instead.
    // This will only be the case for large (e.g. 8192x8192) textures from custom texture packs.
    CD3DX12_HEAP_PROPERTIES hprops(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC rdesc = CD3DX12_RESOURCE_DESC::Buffer(upload_size);
    CheckHR(D3D::device->CreateCommittedResource(&hprops, D3D12_HEAP_FLAG_NONE, &rdesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&upload_buffer)));
    D3D12_RANGE read_range = {};
    CheckHR(upload_buffer->Map(0, &read_range, reinterpret_cast<void**>(&dest_data)));
  }
  else
  {
    if (!s_texture_upload_stream_buffer)
      s_texture_upload_stream_buffer = std::make_unique<D3DStreamBuffer>(
          INITIAL_TEXTURE_UPLOAD_BUFFER_SIZE, MAXIMUM_TEXTURE_UPLOAD_BUFFER_SIZE, nullptr);

    bool current_command_list_executed = s_texture_upload_stream_buffer->AllocateSpaceInBuffer(
        upload_size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    if (current_command_list_executed)
    {
      g_renderer->RestoreAPIState();
    }

    upload_buffer = s_texture_upload_stream_buffer->GetBuffer();
    upload_buffer_offset = s_texture_upload_stream_buffer->GetOffsetOfCurrentAllocation();
    dest_data =
        reinterpret_cast<u8*>(s_texture_upload_stream_buffer->GetCPUAddressOfCurrentAllocation());
  }

  ResourceBarrier(current_command_list, texture12, current_resource_state,
                  D3D12_RESOURCE_STATE_COPY_DEST,
                  D3D12CalcSubresource(level, layer, 0, miplevels, layers));

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT upload_footprint = {};
  u32 upload_rows = 0;
  u64 upload_row_size_in_bytes = 0;
  u64 upload_total_bytes = 0;
  auto tdesc = texture12->GetDesc();
  D3D::device->GetCopyableFootprints(
      &tdesc, D3D12CalcSubresource(level, layer, 0, miplevels, layers), 1, upload_buffer_offset,
      &upload_footprint, &upload_rows, &upload_row_size_in_bytes, &upload_total_bytes);

  const u8* src_data = reinterpret_cast<const u8*>(buffer);
  if (src_pitch == upload_footprint.Footprint.RowPitch && src_pitch == upload_row_size_in_bytes)
  {
    memcpy(dest_data, src_data, upload_row_size_in_bytes * upload_rows);
  }
  else
  {
    for (u32 y = 0; y < upload_rows; ++y)
    {
      memcpy(dest_data + upload_footprint.Footprint.RowPitch * y, src_data + src_pitch * y,
             upload_row_size_in_bytes);
    }
  }
  CD3DX12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(
      texture12, D3D12CalcSubresource(level, layer, 0, miplevels, layers));
  CD3DX12_TEXTURE_COPY_LOCATION src =
      CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer, upload_footprint);
  D3D::current_command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

  // Release temporary buffer after commands complete.
  // We block here because otherwise if there was a large number of texture uploads, we may run out
  // of memory.
  if (upload_size > MAXIMUM_TEXTURE_UPLOAD_BUFFER_SIZE)
  {
    D3D::command_list_mgr->ExecuteQueuedWork(true);
    g_renderer->RestoreAPIState();
    D3D12_RANGE write_range = {0, upload_size};
    upload_buffer->Unmap(0, &write_range);
    upload_buffer->Release();
  }
  else if (upload_size > SYNC_TEXTURE_UPLOAD_BUFFER_SIZE)
  {
    // To grant that the texture data is in place to start rendering we have to execute the copy
    // operation now
    D3D::command_list_mgr->ExecuteQueuedWork();
  }
  ResourceBarrier(current_command_list, texture12, D3D12_RESOURCE_STATE_COPY_DEST,
                  current_resource_state, D3D12CalcSubresource(level, layer, 0, miplevels, layers));
}

}  // namespace D3D

D3DTexture2D* D3DTexture2D::Create(u32 width, u32 height, u32 bind, DXGI_FORMAT fmt, u32 levels,
                                   u32 slices, bool cubemap, D3D12_SUBRESOURCE_DATA* data)
{
  ComPtr<ID3D12Resource> texture;

  D3D12_RESOURCE_DESC texdesc = CD3DX12_RESOURCE_DESC::Tex2D(fmt, width, height, slices, levels);

  D3D12_CLEAR_VALUE optimized_clear_value = {};
  optimized_clear_value.Format = fmt;

  if (bind & TEXTURE_BIND_FLAG_RENDER_TARGET)
  {
    texdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    optimized_clear_value.Color[0] = 0.0f;
    optimized_clear_value.Color[1] = 0.0f;
    optimized_clear_value.Color[2] = 0.0f;
    optimized_clear_value.Color[3] = 1.0f;
  }

  if (bind & TEXTURE_BIND_FLAG_DEPTH_STENCIL)
  {
    texdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    optimized_clear_value.DepthStencil.Depth = 0.0f;
    optimized_clear_value.DepthStencil.Stencil = 0;
  }
  CD3DX12_HEAP_PROPERTIES hprop(D3D12_HEAP_TYPE_DEFAULT);
  CheckHR(D3D::device->CreateCommittedResource(
      &hprop, D3D12_HEAP_FLAG_NONE, &texdesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
      &optimized_clear_value, IID_PPV_ARGS(texture.GetAddressOf())));

  D3D::SetDebugObjectName12(texture.Get(), "Texture created via D3DTexture2D::Create");
  D3DTexture2D* ret = new D3DTexture2D(texture.Get(), bind, fmt, DXGI_FORMAT_UNKNOWN,
                                       DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, false, cubemap,
                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  if (data)
  {
    DX12::D3D::ReplaceTexture2D(texture.Get(), reinterpret_cast<const u8*>(data->pData), fmt, width,
                                height, static_cast<u32>(data->RowPitch), 0, 0, levels, slices,
                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  }
  return ret;
}

void D3DTexture2D::AddRef()
{
  m_ref.fetch_add(1);
}

UINT D3DTexture2D::Release()
{
  UINT ref = m_ref.fetch_sub(1);
  --ref;
  if (ref == 0)
  {
    delete this;
  }
  return ref;
}

void D3DTexture2D::InitalizeSRV()
{
  D3D12_SRV_DIMENSION srv_dim = m_multisampled ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY :
                                                 m_cube ? D3D12_SRV_DIMENSION_TEXTURECUBE :
                                                          D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
      m_srv_format,  // DXGI_FORMAT Format
      srv_dim        // D3D12_SRV_DIMENSION ViewDimension
  };

  if (srv_dim != D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY && srv_dim != D3D12_SRV_DIMENSION_TEXTURECUBE)
  {
    srv_desc.Texture2DArray.MipLevels = -1;
    srv_desc.Texture2DArray.MostDetailedMip = 0;
    srv_desc.Texture2DArray.ResourceMinLODClamp = 0;
    srv_desc.Texture2DArray.ArraySize = -1;
  }
  else
  {
    srv_desc.Texture2DMSArray.ArraySize = -1;
  }

  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  D3D::gpu_descriptor_heap_mgr->Allocate(&m_srv_index, &m_srv_cpu, &m_srv_cpu_shadow, &m_srv_gpu);

  D3D::device->CreateShaderResourceView(m_tex.Get(), &srv_desc, m_srv_cpu);
  D3D::device->CreateShaderResourceView(m_tex.Get(), &srv_desc, m_srv_cpu_shadow);
}

void D3DTexture2D::InitalizeDSV()
{
  D3D12_DSV_DIMENSION dsv_dim =
      m_multisampled ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
      m_dsv_format,        // DXGI_FORMAT Format
      dsv_dim,             // D3D12_DSV_DIMENSION
      D3D12_DSV_FLAG_NONE  // D3D12_DSV_FLAG Flags
  };

  if (dsv_dim == D3D12_DSV_DIMENSION_TEXTURE2DARRAY)
    dsv_desc.Texture2DArray.ArraySize = -1;
  else
    dsv_desc.Texture2DMSArray.ArraySize = -1;

  D3D::dsv_descriptor_heap_mgr->Allocate(&m_dsv_index, &m_dsv, nullptr, nullptr);
  D3D::device->CreateDepthStencilView(m_tex.Get(), &dsv_desc, m_dsv);
}

void D3DTexture2D::InitalizeRTV()
{
  D3D12_RTV_DIMENSION rtv_dim =
      m_multisampled ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
  D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
      m_rtv_format,  // DXGI_FORMAT Format
      rtv_dim        // D3D12_RTV_DIMENSION ViewDimension
  };

  if (rtv_dim == D3D12_RTV_DIMENSION_TEXTURE2DARRAY)
    rtv_desc.Texture2DArray.ArraySize = -1;
  else
    rtv_desc.Texture2DMSArray.ArraySize = -1;

  D3D::rtv_descriptor_heap_mgr->Allocate(&m_rtv_index, &m_rtv, nullptr, nullptr);
  D3D::device->CreateRenderTargetView(m_tex.Get(), &rtv_desc, m_rtv);
}

D3DTexture2D::D3DTexture2D(ID3D12Resource* texptr, u32 bind, DXGI_FORMAT fmt,
                           DXGI_FORMAT srv_format, DXGI_FORMAT dsv_format, DXGI_FORMAT rtv_format,
                           bool multisampled, bool cube, D3D12_RESOURCE_STATES resource_state)
    : m_format(fmt), m_tex(texptr), m_srv_format(srv_format), m_dsv_format(dsv_format),
      m_rtv_format(rtv_format), m_resource_state(resource_state), m_multisampled(multisampled),
      m_cube(cube), m_bind_falgs(bind)
{
  if (m_bind_falgs & TEXTURE_BIND_FLAG_SHADER_RESOURCE)
  {
    InitalizeSRV();
  }
  if (m_bind_falgs & TEXTURE_BIND_FLAG_RENDER_TARGET)
  {
    InitalizeRTV();
  }
  if (m_bind_falgs & TEXTURE_BIND_FLAG_DEPTH_STENCIL)
  {
    InitalizeDSV();
  }
}

D3DTexture2D::~D3DTexture2D()
{
  DX12::D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_tex.Detach());
  if (m_bind_falgs & TEXTURE_BIND_FLAG_SHADER_RESOURCE)
  {
    D3D::command_list_mgr->FreeDescriptorAfterCurrentCommandListExecuted(
        D3D::gpu_descriptor_heap_mgr.get(), m_srv_index);
  }
  if (m_bind_falgs & TEXTURE_BIND_FLAG_RENDER_TARGET)
  {
    D3D::command_list_mgr->FreeDescriptorAfterCurrentCommandListExecuted(
        D3D::rtv_descriptor_heap_mgr.get(), m_rtv_index);
  }
  if (m_bind_falgs & TEXTURE_BIND_FLAG_DEPTH_STENCIL)
  {
    D3D::command_list_mgr->FreeDescriptorAfterCurrentCommandListExecuted(
        D3D::dsv_descriptor_heap_mgr.get(), m_dsv_index);
  }
}

}  // namespace DX12
