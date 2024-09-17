// Copyright 2011 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/Align.h"
#include "Common/CommonFuncs.h"

#include "Core/HW/Memmap.h"
#include "VideoBackends/D3D12/D3DBase.h"
#include "VideoBackends/D3D12/D3DBlob.h"
#include "VideoBackends/D3D12/D3DCommandListManager.h"
#include "VideoBackends/D3D12/D3DDescriptorHeapManager.h"
#include "VideoBackends/D3D12/D3DShader.h"
#include "VideoBackends/D3D12/D3DState.h"
#include "VideoBackends/D3D12/D3DUtil.h"
#include "VideoBackends/D3D12/FramebufferManager.h"
#include "VideoBackends/D3D12/PSTextureEncoder.h"
#include "VideoBackends/D3D12/Render.h"
#include "VideoBackends/D3D12/StaticShaderCache.h"
#include "VideoBackends/D3D12/TextureCache.h"

#include "VideoCommon/TextureConversionShader.h"

namespace DX12
{

struct EFBEncodeParams
{
  DWORD SrcLeft;
  DWORD SrcTop;
  DWORD DestWidth;
  DWORD ScaleFactor;
};

PSTextureEncoder::PSTextureEncoder()
{}

void PSTextureEncoder::InitializeRTV()
{
  // Create output render target view
  D3D12_RENDER_TARGET_VIEW_DESC tex_rtv_desc = {
      DXGI_FORMAT_B8G8R8A8_UNORM,    // DXGI_FORMAT Format;
      D3D12_RTV_DIMENSION_TEXTURE2D  // D3D12_RTV_DIMENSION ViewDimension;
  };

  tex_rtv_desc.Texture2D.MipSlice = 0;

  D3D::rtv_descriptor_heap_mgr->Allocate(nullptr, &m_out_rtv_cpu, nullptr, nullptr);
  D3D::device->CreateRenderTargetView(m_out.Get(), &tex_rtv_desc, m_out_rtv_cpu);
}

void PSTextureEncoder::Init()
{
  // Create output texture RGBA format
  D3D12_RESOURCE_DESC out_tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
    DXGI_FORMAT_B8G8R8A8_UNORM,
    EFB_WIDTH * 4,
    EFB_HEIGHT / 4,
    1,
    0,
    1,
    0,
    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
  );

  D3D12_CLEAR_VALUE optimized_clear_value = { DXGI_FORMAT_B8G8R8A8_UNORM,{ 0.0f, 0.0f, 0.0f, 1.0f } };
  CD3DX12_HEAP_PROPERTIES hprops(D3D12_HEAP_TYPE_DEFAULT);
  CheckHR(
    D3D::device->CreateCommittedResource(
      &hprops,
      D3D12_HEAP_FLAG_NONE,
      &out_tex_desc,
      D3D12_RESOURCE_STATE_COPY_SOURCE,
      &optimized_clear_value,
      IID_PPV_ARGS(m_out.ReleaseAndGetAddressOf())
    )
  );

  D3D::SetDebugObjectName12(m_out.Get(), "efb encoder output texture");

  InitializeRTV();
  hprops = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
  auto rdesc = CD3DX12_RESOURCE_DESC::Buffer(
    Common::AlignUpSizePow2(static_cast<unsigned int>(out_tex_desc.Width) * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) *
    out_tex_desc.Height
  );
  // Create output staging buffer
  CheckHR(
    D3D::device->CreateCommittedResource(
      &hprops,
      D3D12_HEAP_FLAG_NONE,
      &rdesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(m_out_readback_buffer.ReleaseAndGetAddressOf())
    )
  );

  D3D::SetDebugObjectName12(m_out_readback_buffer.Get(), "efb encoder output staging buffer");

  // Create constant buffer for uploading data to shaders. Need to align to 256 bytes.
  unsigned int encode_params_buffer_size = (sizeof(EFBEncodeParams) + 0xff) & ~0xff;
  hprops = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  rdesc = CD3DX12_RESOURCE_DESC::Buffer(encode_params_buffer_size);
  CheckHR(
    D3D::device->CreateCommittedResource(
      &hprops,
      D3D12_HEAP_FLAG_NONE,
      &rdesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(m_encode_params_buffer.ReleaseAndGetAddressOf())
    )
  );

  D3D::SetDebugObjectName12(m_encode_params_buffer.Get(), "efb encoder params buffer");
  // NOTE: This upload buffer is okay to overwrite each time, since we block until completion when it's used anyway.
  D3D12_RANGE read_range = {};
  CheckHR(m_encode_params_buffer->Map(0, &read_range, &m_encode_params_buffer_data));

  m_ready = true;
}

void PSTextureEncoder::Shutdown()
{
  m_ready = false;
  D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_out.Detach());
  D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_out_readback_buffer.Detach());
  D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_encode_params_buffer.Detach());
  for (auto& it : m_shader_blobs)
  {
    SAFE_RELEASE(it);
  }
  m_shader_blobs.clear();
  m_encoding_shaders.clear();
}

void PSTextureEncoder::Encode(u8* dst, const EFBCopyFormat& format, u32 native_width,
  u32 bytes_per_row, u32 num_blocks_y, u32 memory_stride,
  bool is_depth_copy, const EFBRectangle& src_rect, bool scale_by_half)
{
  if (!m_ready) // Make sure we initialized OK
    return;

  D3D::command_list_mgr->CPUAccessNotify();

  // Resolve MSAA targets before copying.
  D3DTexture2D* efb_source = is_depth_copy ?
    FramebufferManager::GetResolvedEFBDepthTexture() :
    // EXISTINGD3D11TODO: Instead of resolving EFB, it would be better to pick out a
    // single sample from each pixel. The game may break if it isn't
    // expecting the blurred edges around multisampled shapes.
    FramebufferManager::GetResolvedEFBColorTexture();

  // GetResolvedEFBDepthTexture will set the render targets, when MSAA is enabled 
  // (since it needs to do a manual depth resolve). So make sure to set the RTs
  // afterwards.

  const u32 words_per_row = bytes_per_row / sizeof(u32);

  D3D::SetViewportAndScissor(0, 0, words_per_row, num_blocks_y);

  constexpr EFBRectangle full_src_rect(0, 0, EFB_WIDTH, EFB_HEIGHT);

  TargetRectangle target_rect = g_renderer->ConvertEFBRectangle(full_src_rect);

  D3D::ResourceBarrier(D3D::current_command_list, m_out.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
  D3D::current_command_list->OMSetRenderTargets(1, &m_out_rtv_cpu, FALSE, nullptr);

  EFBEncodeParams params;
  params.SrcLeft = src_rect.left;
  params.SrcTop = src_rect.top;
  params.DestWidth = native_width;
  params.ScaleFactor = scale_by_half ? 2 : 1;

  memcpy(m_encode_params_buffer_data, &params, sizeof(params));
  D3D::current_command_list->SetGraphicsRootConstantBufferView(
    DESCRIPTOR_TABLE_PS_CBVONE,
    m_encode_params_buffer->GetGPUVirtualAddress()
  );

  D3D::command_list_mgr->SetCommandListDirtyState(COMMAND_LIST_STATE_PS_CBV, true);

  // Use linear filtering if (bScaleByHalf), use point filtering otherwise
  if (scale_by_half || g_ActiveConfig.iEFBScale != SCALE_1X)
    D3D::SetLinearCopySampler();
  else
    D3D::SetPointCopySampler();

  D3D::DrawShadedTexQuad(efb_source,
    target_rect.AsRECT(),
    g_renderer->GetTargetWidth(),
    g_renderer->GetTargetHeight(),
    GetEncodingPixelShader(format),
    StaticShaderCache::GetSimpleVertexShader(),
    StaticShaderCache::GetSimpleVertexShaderInputLayout(),
    D3D12_SHADER_BYTECODE(),
    0,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    false,
    false /* Render target is not multisampled */
  );

  // Copy to staging buffer
  D3D12_BOX src_box = CD3DX12_BOX(0, 0, 0, words_per_row, num_blocks_y, 1);

  D3D12_TEXTURE_COPY_LOCATION dst_location = {};
  dst_location.pResource = m_out_readback_buffer.Get();
  dst_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dst_location.PlacedFootprint.Offset = 0;
  dst_location.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  dst_location.PlacedFootprint.Footprint.Width = EFB_WIDTH * 4;
  dst_location.PlacedFootprint.Footprint.Height = EFB_HEIGHT / 4;
  dst_location.PlacedFootprint.Footprint.Depth = 1;
  dst_location.PlacedFootprint.Footprint.RowPitch = Common::AlignUpSizePow2(dst_location.PlacedFootprint.Footprint.Width * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

  D3D12_TEXTURE_COPY_LOCATION src_location = {};
  src_location.pResource = m_out.Get();
  src_location.SubresourceIndex = 0;
  src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

  D3D::ResourceBarrier(D3D::current_command_list, m_out.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
  D3D::current_command_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, &src_box);

  // State is automatically restored after executing command list.
  D3D::command_list_mgr->ExecuteQueuedWork(true);

  // Transfer staging buffer to GameCube/Wii RAM
  void* readback_data_map;
  D3D12_RANGE read_range = { 0, dst_location.PlacedFootprint.Footprint.RowPitch * num_blocks_y };
  CheckHR(m_out_readback_buffer->Map(0, &read_range, &readback_data_map));

  u8* src = static_cast<u8*>(readback_data_map);
  u32 read_stride = std::min(bytes_per_row, dst_location.PlacedFootprint.Footprint.RowPitch);
  for (unsigned int y = 0; y < num_blocks_y; ++y)
  {
    memcpy(dst, src, read_stride);

    dst += memory_stride;
    src += dst_location.PlacedFootprint.Footprint.RowPitch;
  }
  D3D12_RANGE write_range = {};
  m_out_readback_buffer->Unmap(0, &write_range);
}

D3D12_SHADER_BYTECODE PSTextureEncoder::GetEncodingPixelShader(const EFBCopyFormat& format)
{
  auto iter = m_encoding_shaders.find(format);
  if (iter != m_encoding_shaders.end())
    return iter->second;

  D3DBlob* bytecode = nullptr;
  const char* shader = TextureConversionShader::GenerateEncodingShader(format, API_TYPE::API_D3D11);
  if (!D3D::CompilePixelShader(shader, &bytecode))
  {
    PanicAlert("Failed to compile texture encoding shader.");
    m_encoding_shaders[format] = {};
    return {};
  }

  D3D12_SHADER_BYTECODE new_shader = {
      bytecode->Data(),
      bytecode->Size()
  };
  m_encoding_shaders.emplace(format, new_shader);

  // Keep track of the ID3DBlobs, so we can free them upon shutdown.
  m_shader_blobs.push_back(bytecode);

  return new_shader;
}

}
