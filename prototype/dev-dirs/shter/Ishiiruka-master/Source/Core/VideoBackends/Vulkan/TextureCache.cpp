// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoBackends/Vulkan/TextureCache.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "Common/Assert.h"
#include "Common/CommonFuncs.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"

#include "VideoBackends/Vulkan/CommandBufferManager.h"
#include "VideoBackends/Vulkan/FramebufferManager.h"
#include "VideoBackends/Vulkan/ObjectCache.h"
#include "VideoBackends/Vulkan/Renderer.h"
#include "VideoBackends/Vulkan/StagingTexture2D.h"
#include "VideoBackends/Vulkan/StateTracker.h"
#include "VideoBackends/Vulkan/StreamBuffer.h"
#include "VideoBackends/Vulkan/Texture2D.h"
#include "VideoBackends/Vulkan/TextureConverter.h"
#include "VideoBackends/Vulkan/Util.h"
#include "VideoBackends/Vulkan/VulkanContext.h"
#include "VideoBackends/Vulkan/VKTexture.h"

#include "VideoCommon/ImageWrite.h"

namespace Vulkan
{
TextureCache::TextureCache()
{
  for (size_t i = 0; i < m_render_pass.size(); i++)
  {
    m_render_pass[i] = VK_NULL_HANDLE;
  }
  m_scaler = std::make_unique<TextureScaler>();
}

TextureCache::~TextureCache()
{
  for (size_t i = 0; i < m_render_pass.size(); i++)
  {
    if (m_render_pass[i] != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass(g_vulkan_context->GetDevice(), m_render_pass[i], nullptr);
      m_render_pass[i] = VK_NULL_HANDLE;
    }
  }
  TextureCache::DeleteShaders();
  m_scaler.reset();
}

TextureCache* TextureCache::GetInstance()
{
  return static_cast<TextureCache*>(g_texture_cache.get());
}

bool TextureCache::Initialize()
{
  m_texture_upload_buffer =
    StreamBuffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, INITIAL_TEXTURE_UPLOAD_BUFFER_SIZE,
      MAXIMUM_TEXTURE_UPLOAD_BUFFER_SIZE);
  if (!m_texture_upload_buffer)
  {
    PanicAlert("Failed to create texture upload buffer");
    return false;
  }

  if (!CreateRenderPasses())
  {
    PanicAlert("Failed to create copy render pass");
    return false;
  }

  m_texture_converter = std::make_unique<TextureConverter>();
  if (!m_texture_converter->Initialize())
  {
    PanicAlert("Failed to initialize texture converter.");
    return false;
  }

  if (!CompileShaders())
  {
    PanicAlert("Failed to compile one or more shaders");
    return false;
  }

  return true;
}

bool TextureCache::Palettize(TCacheEntry* _entry, const TCacheEntry* base_entry)
{
  TCacheEntry* entry = static_cast<TCacheEntry*>(_entry);
  const TCacheEntry* unconverted = static_cast<const TCacheEntry*>(base_entry);
  VKTexture* dst_tex = static_cast<VKTexture*>(_entry->texture.get());
  m_texture_converter->ConvertTexture(entry, unconverted, GetRenderPass(dst_tex->GetRawTexIdentifier()->GetFormat()), m_pallette, m_pallette_format, m_pallette_size);
  static_cast<VKTexture*>(base_entry->texture.get())
    ->GetRawTexIdentifier()
    ->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  dst_tex->GetRawTexIdentifier()
    ->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  return true;
}

void TextureCache::LoadLut(u32 lutFmt, void* addr, u32 size)
{
  m_pallette = addr;
  m_pallette_format = (TlutFormat)lutFmt;
  m_pallette_size = size;
}

void TextureCache::CopyEFB(u8* dst, const EFBCopyFormat& format, u32 native_width, u32 bytes_per_row,
  u32 num_blocks_y, u32 memory_stride,
  bool is_depth_copy, const EFBRectangle& src_rect, bool scale_by_half)
{
  // Flush EFB pokes first, as they're expected to be included.
  FramebufferManager::GetInstance()->FlushEFBPokes();

  // MSAA case where we need to resolve first.
  // An out-of-bounds source region is valid here, and fine for the draw (since it is converted
  // to texture coordinates), but it's not valid to resolve an out-of-range rectangle.
  TargetRectangle scaled_src_rect = g_renderer->ConvertEFBRectangle(src_rect);
  VkRect2D region = { { scaled_src_rect.left, scaled_src_rect.top },
  { static_cast<u32>(scaled_src_rect.GetWidth()),
      static_cast<u32>(scaled_src_rect.GetHeight()) } };

  region = Util::ClampRect2D(region, FramebufferManager::GetInstance()->GetEFBWidth(),
    FramebufferManager::GetInstance()->GetEFBHeight());
  Texture2D* src_texture;
  if (is_depth_copy)
    src_texture = FramebufferManager::GetInstance()->ResolveEFBDepthTexture(region);
  else
    src_texture = FramebufferManager::GetInstance()->ResolveEFBColorTexture(region);

  // End render pass before barrier (since we have no self-dependencies).
  // The barrier has to happen after the render pass, not inside it, as we are going to be
  // reading from the texture immediately afterwards.
  StateTracker::GetInstance()->EndRenderPass();
  StateTracker::GetInstance()->OnReadback();

  // Transition to shader resource before reading.
  VkImageLayout original_layout = src_texture->GetLayout();
  src_texture->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(),
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  m_texture_converter->EncodeTextureToMemory(src_texture->GetView(), dst, format, native_width,
    bytes_per_row, num_blocks_y, memory_stride,
    is_depth_copy, src_rect, scale_by_half);

  // Transition back to original state
  src_texture->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(), original_layout);
}

HostTextureFormat TextureCache::GetHostTextureFormat(const s32 texformat, const TlutFormat tlutfmt, u32 width, u32 height)
{
  const bool compressed_supported = ((width & 3) == 0) && ((height & 3) == 0);
  HostTextureFormat pcfmt = TexDecoder::GetHostTextureFormat(texformat, tlutfmt, compressed_supported);
  pcfmt = !g_ActiveConfig.backend_info.bSupportedFormats[pcfmt] ? PC_TEX_FMT_RGBA32 : pcfmt;
  return pcfmt;
}

bool TextureCache::SupportsGPUTextureDecode(TextureFormat format, TlutFormat palette_format)
{
  return m_texture_converter->SupportsTextureDecoding(format, palette_format);
}

std::unique_ptr<HostTexture> TextureCache::CreateTexture(const TextureConfig& config)
{
  return VKTexture::Create(config);
}

bool TextureCache::CreateRenderPasses()
{
  VkAttachmentDescription update_attachment = {
      0,
      TEXTURECACHE_TEXTURE_FORMAT,
      VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  static constexpr VkAttachmentReference color_attachment_reference = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  static constexpr VkSubpassDescription subpass_description = {
      0,       VK_PIPELINE_BIND_POINT_GRAPHICS,
      0,       nullptr,
      1,       &color_attachment_reference,
      nullptr, nullptr,
      0,       nullptr };

  VkRenderPassCreateInfo update_info =
  {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      nullptr,
      0,
      1,
      &update_attachment,
      1,
      &subpass_description,
      0,
      nullptr
  };

  VkResult res = vkCreateRenderPass(g_vulkan_context->GetDevice(), &update_info, nullptr,
    &m_render_pass[0]);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateRenderPass failed: ");
    return false;
  }
  update_attachment.format = VK_FORMAT_D32_SFLOAT;
  res = vkCreateRenderPass(g_vulkan_context->GetDevice(), &update_info, nullptr,
    &m_render_pass[1]);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateRenderPass failed: ");
    return false;
  }

  update_attachment.format = VK_FORMAT_R32_SFLOAT;
  res = vkCreateRenderPass(g_vulkan_context->GetDevice(), &update_info, nullptr,
    &m_render_pass[2]);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateRenderPass failed: ");
    return false;
  }

  update_attachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  res = vkCreateRenderPass(g_vulkan_context->GetDevice(), &update_info, nullptr,
    &m_render_pass[3]);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateRenderPass failed: ");
    return false;
  }

  update_attachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  res = vkCreateRenderPass(g_vulkan_context->GetDevice(), &update_info, nullptr,
    &m_render_pass[4]);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateRenderPass failed: ");
    return false;
  }

  return true;
}

bool TextureCache::DecodeTextureOnGPU(HostTexture* dst, u32 dst_level, const u8* data,
  u32 data_size, TextureFormat tformat, u32 width, u32 height,
  u32 aligned_width, u32 aligned_height, u32 row_stride,
  const u8* palette, TlutFormat palette_format)
{
  return m_texture_converter->DecodeTexture(dst, dst_level, data, data_size,
    tformat, width, height, aligned_width, aligned_height,
    row_stride, palette, palette_format);
}

void TextureCache::CopyEFBToCacheEntry(
  TextureCacheBase::TCacheEntry* entry, bool is_depth_copy, const EFBRectangle& src_rect,
  bool scale_by_half, u32 cbuf_id, const float* colmat, u32 width, u32 height)
{
  VKTexture* texture = static_cast<VKTexture*>(entry->texture.get());
  // A better way of doing this would be nice.
  FramebufferManager* framebuffer_mgr =
    static_cast<FramebufferManager*>(g_framebuffer_manager.get());
  TargetRectangle scaled_src_rect = g_renderer->ConvertEFBRectangle(src_rect);

  // Flush EFB pokes first, as they're expected to be included.
  framebuffer_mgr->FlushEFBPokes();

  // Has to be flagged as a render target.
  ASSERT(texture->GetFramebuffer() != VK_NULL_HANDLE);

  // Can't be done in a render pass, since we're doing our own render pass!
  VkCommandBuffer command_buffer = g_command_buffer_mgr->GetCurrentCommandBuffer();
  StateTracker::GetInstance()->EndRenderPass();

  // Transition EFB to shader resource before binding.
  // An out-of-bounds source region is valid here, and fine for the draw (since it is converted
  // to texture coordinates), but it's not valid to resolve an out-of-range rectangle.
  VkRect2D region =
  {
      { scaled_src_rect.left, scaled_src_rect.top },
      { static_cast<u32>(scaled_src_rect.GetWidth()), static_cast<u32>(scaled_src_rect.GetHeight()) }
  };
  region = Util::ClampRect2D(region, FramebufferManager::GetInstance()->GetEFBWidth(),
    FramebufferManager::GetInstance()->GetEFBHeight());

  Texture2D* src_texture;
  if (is_depth_copy)
    src_texture = FramebufferManager::GetInstance()->ResolveEFBDepthTexture(region);
  else
    src_texture = FramebufferManager::GetInstance()->ResolveEFBColorTexture(region);

  VkSampler src_sampler =
    scale_by_half ? g_object_cache->GetLinearSampler() : g_object_cache->GetPointSampler();
  VkImageLayout original_layout = src_texture->GetLayout();
  src_texture->TransitionToLayout(command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  texture->GetRawTexIdentifier()->TransitionToLayout(command_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  UtilityShaderDraw draw(
    command_buffer, g_object_cache->GetPipelineLayout(PIPELINE_LAYOUT_PUSH_CONSTANT),
    GetRenderPass(texture->GetRawTexIdentifier()->GetFormat()), g_shader_cache->GetPassthroughVertexShader(),
    g_shader_cache->GetPassthroughGeometryShader(),
    is_depth_copy ? m_efb_depth_to_tex_shader : m_efb_color_to_tex_shader);

  draw.SetPushConstants(colmat, (is_depth_copy ? sizeof(float) * 20 : sizeof(float) * 28));
  draw.SetPSSampler(0, src_texture->GetView(), src_sampler);

  VkRect2D dest_region = { { 0, 0 },{ width, height } };

  draw.BeginRenderPass(texture->GetRawTexIdentifier()->GetFrameBuffer(), dest_region);

  draw.DrawQuad(0, 0, texture->GetConfig().width, texture->GetConfig().height, scaled_src_rect.left, scaled_src_rect.top, 0,
    scaled_src_rect.GetWidth(), scaled_src_rect.GetHeight(),
    framebuffer_mgr->GetEFBWidth(), framebuffer_mgr->GetEFBHeight());

  draw.EndRenderPass();

  // We touched everything, so put it back.
  StateTracker::GetInstance()->SetPendingRebind();

  // Transition the EFB back to its original layout.
  src_texture->TransitionToLayout(command_buffer, original_layout);

  // Render pass transitions texture to SHADER_READ_ONLY.
  texture->GetRawTexIdentifier()->TransitionToLayout(command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

bool TextureCache::CompileShaders()
{
  static const char COPY_SHADER_SOURCE[] = R"(
    layout(set = 1, binding = 0) uniform sampler2DArray samp0;

    layout(location = 0) in float3 uv0;
    layout(location = 1) in float4 col0;
    layout(location = 0) out float4 ocol0;

    void main()
    {
      ocol0 = texture(samp0, uv0);
    }
  )";

  static const char EFB_COLOR_TO_TEX_SOURCE[] = R"(
    SAMPLER_BINDING(0) uniform sampler2DArray samp0;

    layout(std140, push_constant) uniform PSBlock
    {
      vec4 colmat[7];
    } C;

    layout(location = 0) in vec3 uv0;
    layout(location = 1) in vec4 col0;
    layout(location = 0) out vec4 ocol0;

    void main()
    {
      float4 texcol = texture(samp0, uv0);
      texcol = floor(texcol * C.colmat[5]) * C.colmat[6];
      ocol0 = texcol * mat4(C.colmat[0], C.colmat[1], C.colmat[2], C.colmat[3]) + C.colmat[4];
    }
  )";

  static const char EFB_DEPTH_TO_TEX_SOURCE[] = R"(
    SAMPLER_BINDING(0) uniform sampler2DArray samp0;

    layout(std140, push_constant) uniform PSBlock
    {
      vec4 colmat[5];
    } C;

    layout(location = 0) in vec3 uv0;
    layout(location = 1) in vec4 col0;
    layout(location = 0) out vec4 ocol0;

    void main()
    {
      #if MONO_DEPTH
        vec4 texcol = texture(samp0, vec3(uv0.xy, 0.0f));
      #else
        vec4 texcol = texture(samp0, uv0);
      #endif
      int depth = int((1.0 - texcol.x) * 16777216.0);

      // Convert to Z24 format
      ivec4 workspace;
      workspace.r = (depth >> 16) & 255;
      workspace.g = (depth >> 8) & 255;
      workspace.b = depth & 255;

      // Convert to Z4 format
      workspace.a = (depth >> 16) & 0xF0;

      // Normalize components to [0.0..1.0]
      texcol = vec4(workspace) / 255.0;

      ocol0 = texcol * mat4(C.colmat[0], C.colmat[1], C.colmat[2], C.colmat[3]) + C.colmat[4];
    }
  )";

  std::string header = g_shader_cache->GetUtilityShaderHeader();
  std::string source;

  source = header + COPY_SHADER_SOURCE;
  m_copy_shader = Util::CompileAndCreateFragmentShader(source);

  source = header + EFB_COLOR_TO_TEX_SOURCE;
  m_efb_color_to_tex_shader = Util::CompileAndCreateFragmentShader(source);

  if (g_ActiveConfig.bStereoEFBMonoDepth)
    source = header + "#define MONO_DEPTH 1\n" + EFB_DEPTH_TO_TEX_SOURCE;
  else
    source = header + EFB_DEPTH_TO_TEX_SOURCE;
  m_efb_depth_to_tex_shader = Util::CompileAndCreateFragmentShader(source);

  return (m_copy_shader != VK_NULL_HANDLE && m_efb_color_to_tex_shader != VK_NULL_HANDLE &&
    m_efb_depth_to_tex_shader != VK_NULL_HANDLE);
}

void TextureCache::DeleteShaders()
{
  // It is safe to destroy shader modules after they are consumed by creating a pipeline.
  // Therefore, no matter where this function is called from, it won't cause an issue due to
  // pending commands, although at the time of writing should only be called at the end of
  // a frame. See Vulkan spec, section 2.3.1. Object Lifetime.
  if (m_copy_shader != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(g_vulkan_context->GetDevice(), m_copy_shader, nullptr);
    m_copy_shader = VK_NULL_HANDLE;
  }
  if (m_efb_color_to_tex_shader != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(g_vulkan_context->GetDevice(), m_efb_color_to_tex_shader, nullptr);
    m_efb_color_to_tex_shader = VK_NULL_HANDLE;
  }
  if (m_efb_depth_to_tex_shader != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(g_vulkan_context->GetDevice(), m_efb_depth_to_tex_shader, nullptr);
    m_efb_depth_to_tex_shader = VK_NULL_HANDLE;
  }
}

}  // namespace Vulkan
