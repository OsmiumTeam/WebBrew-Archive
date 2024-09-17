// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstring>

#include "VideoBackends/Vulkan/VertexManager.h"
#include "VideoBackends/Vulkan/BoundingBox.h"
#include "VideoBackends/Vulkan/CommandBufferManager.h"
#include "VideoBackends/Vulkan/FramebufferManager.h"
#include "VideoBackends/Vulkan/PerfQuery.h"
#include "VideoBackends/Vulkan/Renderer.h"
#include "VideoBackends/Vulkan/StateTracker.h"
#include "VideoBackends/Vulkan/StreamBuffer.h"
#include "VideoBackends/Vulkan/Util.h"
#include "VideoBackends/Vulkan/VertexFormat.h"
#include "VideoBackends/Vulkan/VulkanContext.h"

#include "VideoCommon/BoundingBox.h"
#include "VideoCommon/IndexGenerator.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VideoConfig.h"

namespace Vulkan
{
// TODO: Clean up this mess
constexpr size_t INITIAL_VERTEX_BUFFER_SIZE = VertexManager::MAXVBUFFERSIZE * 2;
constexpr size_t MAX_VERTEX_BUFFER_SIZE = VertexManager::MAXVBUFFERSIZE * 16;
constexpr size_t INITIAL_INDEX_BUFFER_SIZE = VertexManager::MAXIBUFFERSIZE * sizeof(u16) * 2;
constexpr size_t MAX_INDEX_BUFFER_SIZE = VertexManager::MAXIBUFFERSIZE * sizeof(u16) * 16;

VertexManager::VertexManager()
  : m_cpu_vertex_buffer(MAXVBUFFERSIZE), m_cpu_index_buffer(MAXIBUFFERSIZE)
{
}

VertexManager::~VertexManager()
{
}

VertexManager* VertexManager::GetInstance()
{
  return static_cast<VertexManager*>(g_vertex_manager.get());
}

bool VertexManager::Initialize()
{
  m_vertex_stream_buffer = StreamBuffer::Create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    INITIAL_VERTEX_BUFFER_SIZE, MAX_VERTEX_BUFFER_SIZE);

  m_index_stream_buffer = StreamBuffer::Create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    INITIAL_INDEX_BUFFER_SIZE, MAX_INDEX_BUFFER_SIZE);

  if (!m_vertex_stream_buffer || !m_index_stream_buffer)
  {
    PanicAlert("Failed to allocate streaming buffers");
    return false;
  }

  return true;
}

std::unique_ptr<NativeVertexFormat>
VertexManager::CreateNativeVertexFormat(const PortableVertexDeclaration& vtx_decl)
{
  return std::make_unique<VertexFormat>(vtx_decl);
}

void VertexManager::PrepareDrawBuffers(u32 stride)
{
  size_t vertex_data_size = IndexGenerator::GetNumVerts() * stride;
  size_t index_data_size = IndexGenerator::GetIndexLen() * sizeof(u16);

  // Attempt to allocate from buffers
  bool has_vbuffer_allocation = m_vertex_stream_buffer->ReserveMemory(vertex_data_size, stride);
  bool has_ibuffer_allocation = m_index_stream_buffer->ReserveMemory(index_data_size, sizeof(u16));
  if (!has_vbuffer_allocation || !has_ibuffer_allocation)
  {
    // Flush any pending commands first, so that we can wait on the fences
    WARN_LOG(VIDEO, "Executing command list while waiting for space in vertex/index buffer");
    Util::ExecuteCurrentCommandsAndRestoreState(false);

    // Attempt to allocate again, this may cause a fence wait
    if (!has_vbuffer_allocation)
      has_vbuffer_allocation = m_vertex_stream_buffer->ReserveMemory(vertex_data_size, stride);
    if (!has_ibuffer_allocation)
      has_ibuffer_allocation = m_index_stream_buffer->ReserveMemory(index_data_size, sizeof(u16));

    // If we still failed, that means the allocation was too large and will never succeed, so panic
    if (!has_vbuffer_allocation || !has_ibuffer_allocation)
      PanicAlert("Failed to allocate space in streaming buffers for pending draw");
  }

  std::memcpy(m_vertex_stream_buffer->GetCurrentHostPointer(), m_cpu_vertex_buffer.data(), vertex_data_size);
  std::memcpy(m_index_stream_buffer->GetCurrentHostPointer(), m_cpu_index_buffer.data(), index_data_size);

  // Update base indices
  m_current_draw_base_vertex =
    static_cast<u32>(m_vertex_stream_buffer->GetCurrentOffset() / stride);
  m_current_draw_base_index =
    static_cast<u32>(m_index_stream_buffer->GetCurrentOffset() / sizeof(u16));

  m_vertex_stream_buffer->CommitMemory(vertex_data_size);
  m_index_stream_buffer->CommitMemory(index_data_size);

  ADDSTAT(stats.thisFrame.bytesVertexStreamed, static_cast<int>(vertex_data_size));
  ADDSTAT(stats.thisFrame.bytesIndexStreamed, static_cast<int>(index_data_size));

  StateTracker::GetInstance()->SetVertexBuffer(m_vertex_stream_buffer->GetBuffer(), 0);
  StateTracker::GetInstance()->SetIndexBuffer(m_index_stream_buffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
}

void VertexManager::ResetBuffer(u32 stride)
{
  m_pCurBufferPointer = m_pBaseBufferPointer = m_cpu_vertex_buffer.data();
  m_pEndBufferPointer = m_pBaseBufferPointer + m_cpu_vertex_buffer.size();
  IndexGenerator::Start(m_cpu_index_buffer.data());
}

u16* VertexManager::GetIndexBuffer()
{
  return reinterpret_cast<u16*>(IndexGenerator::GetBasePointer());
}

void VertexManager::vFlush(bool use_dst_alpha)
{
  const VertexFormat* vertex_format =
    static_cast<VertexFormat*>(VertexLoaderManager::GetCurrentVertexFormat());
  u32 vertex_stride = vertex_format->GetVertexStride();

  // Figure out the number of indices to draw
  u32 index_count = IndexGenerator::GetIndexLen();

  // Update assembly state
  StateTracker::GetInstance()->SetVertexFormat(vertex_format);

  // Can we do single-pass dst alpha?
  PIXEL_SHADER_RENDER_MODE dstalpha_mode = PIXEL_SHADER_RENDER_MODE::PSRM_DEFAULT;
  if (use_dst_alpha && g_vulkan_context->SupportsDualSourceBlend())
    dstalpha_mode = PIXEL_SHADER_RENDER_MODE::PSRM_DUAL_SOURCE_BLEND;

  // Check for any shader stage changes
  StateTracker::GetInstance()->CheckForShaderChanges(m_current_primitive_type, VertexLoaderManager::g_current_components, dstalpha_mode);

  // Update any changed constants
  StateTracker::GetInstance()->UpdateVertexShaderConstants();
  StateTracker::GetInstance()->UpdateGeometryShaderConstants();
  StateTracker::GetInstance()->UpdatePixelShaderConstants();

  // Commit memory to device.
  // NOTE: This must be done after constant upload, as a constant buffer overrun can cause
  // the current command buffer to be executed, and we want the buffer space to be associated
  // with the command buffer that has the corresponding draw.
  PrepareDrawBuffers(vertex_stride);

  // Flush all EFB pokes and invalidate the peek cache.
  FramebufferManager::GetInstance()->InvalidatePeekCache();
  FramebufferManager::GetInstance()->FlushEFBPokes();

  // If bounding box is enabled, we need to flush any changes first, then invalidate what we have.
  if (g_vulkan_context->SupportsBoundingBox())
  {
    BoundingBox* bounding_box = Renderer::GetInstance()->GetBoundingBox();
    bool bounding_box_enabled = (::BoundingBox::active && g_ActiveConfig.iBBoxMode == BBoxGPU);
    if (bounding_box_enabled)
    {
      bounding_box->Flush();
      bounding_box->Invalidate();
    }

    // Update which descriptor set/pipeline layout to use.
    StateTracker::GetInstance()->SetBBoxEnable(bounding_box_enabled);
  }

  // Bind all pending state to the command buffer
  if (!StateTracker::GetInstance()->Bind())
  {
    WARN_LOG(VIDEO, "Skipped draw of %u indices", index_count);
    return;
  }
  if (PerfQueryBase::ShouldEmulate())
    static_cast<PerfQuery*>(g_perf_query.get())->StartQuery();
  // Execute the draw
  vkCmdDrawIndexed(g_command_buffer_mgr->GetCurrentCommandBuffer(), index_count, 1,
    m_current_draw_base_index, m_current_draw_base_vertex, 0);
  if (PerfQueryBase::ShouldEmulate())
    static_cast<PerfQuery*>(g_perf_query.get())->EndQuery();

  // If the GPU does not support dual-source blending, we can approximate the effect by drawing
  // the object a second time, with the write mask set to alpha only using a shader that outputs
  // the destination/constant alpha value (which would normally be SRC_COLOR.a).
  //
  // This is also used when logic ops and destination alpha is enabled, since we can't enable
  // blending and logic ops concurrently (and the logical operation applies to all channels).
  bool logic_op_enabled = bpmem.blendmode.logicopenable.Value() && !bpmem.blendmode.blendenable.Value();
  if (use_dst_alpha && (!g_vulkan_context->SupportsDualSourceBlend() || logic_op_enabled))
  {
    StateTracker::GetInstance()->CheckForShaderChanges(m_current_primitive_type, VertexLoaderManager::g_current_components, PIXEL_SHADER_RENDER_MODE::PSRM_ALPHA_PASS);
    if (!StateTracker::GetInstance()->Bind())
    {
      WARN_LOG(VIDEO, "Skipped draw of %u indices (alpha pass)", index_count);
      return;
    }

    vkCmdDrawIndexed(g_command_buffer_mgr->GetCurrentCommandBuffer(), index_count, 1,
      m_current_draw_base_index, m_current_draw_base_vertex, 0);
  }

  StateTracker::GetInstance()->OnDraw();
}

}  // namespace Vulkan
