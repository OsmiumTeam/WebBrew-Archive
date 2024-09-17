// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>

#include "VideoBackends/D3D12/D3DBase.h"
#include "VideoBackends/D3D12/D3DCommandListManager.h"
#include "VideoBackends/D3D12/D3DStreamBuffer.h"
#include "VideoBackends/D3D12/D3DUtil.h"

namespace DX12
{

D3DStreamBuffer::D3DStreamBuffer(size_t initial_size, size_t max_size, bool* buffer_reallocation_notification) :
  m_buffer_size(initial_size),
  m_buffer_max_size(max_size),
  m_buffer_reallocation_notification(buffer_reallocation_notification)
{
  CHECK(initial_size <= max_size, "Error: Initial size for D3DStreamBuffer is greater than max_size.");

  AllocateBuffer(initial_size);

  // Register for callback from D3DCommandListManager each time a fence is queued to be signaled.
  m_buffer_tracking_fence = D3D::command_list_mgr->RegisterQueueFenceCallback(this, &D3DStreamBuffer::QueueFenceCallback);
}

D3DStreamBuffer::~D3DStreamBuffer()
{
  D3D::command_list_mgr->RemoveQueueFenceCallback(this);
  D3D12_RANGE write_range = { 0, m_buffer_size };
  m_buffer->Unmap(0, &write_range);
  D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_buffer.Detach());
}

// Function returns true if (worst case), needed to flush existing command list in order to
// ensure the GPU finished with current use of buffer. The calling function will need to take
// care to reset GPU state to what it was previously.

// Obviously this is non-performant, so the buffer max_size should be large enough to
// ensure this never happens.
bool D3DStreamBuffer::AllocateSpaceInBuffer(size_t allocation_size, size_t alignment, bool allow_execute)
{
  CHECK(allocation_size <= m_buffer_max_size, "Error: Requested allocation size in D3DStreamBuffer is greater than max allowed size of backing buffer.");

  if (alignment && m_buffer_offset > 0)
  {
    size_t padding = m_buffer_offset % alignment;
    size_t offset = m_buffer_offset + alignment - padding;
    // Check for case when adding alignment causes CPU offset to equal GPU offset,
    // which would imply entire buffer is available (if not corrected).
    if (m_buffer_offset < m_buffer_gpu_completion_offset &&
      offset == m_buffer_gpu_completion_offset)
    {
      m_buffer_gpu_completion_offset++;
    }

    m_buffer_offset = offset;

    if (m_buffer_offset > m_buffer_size)
    {
      m_buffer_offset = 0;

      // Correct for case where CPU was about to run into GPU.
      if (m_buffer_gpu_completion_offset == 0)
        m_buffer_gpu_completion_offset = 1;
    }
  }

  // First, check if there is available (not-in-use-by-GPU) space in existing buffer.
  if (AttemptToAllocateOutOfExistingUnusedSpaceInBuffer(allocation_size))
  {
    return false;
  }

  // Slow path. No room at front, or back, due to the GPU still (possibly) accessing parts of the buffer.
  // Resize if possible, else stall.
  bool command_list_executed = AttemptBufferResizeOrElseStall(allocation_size, allow_execute);

  return command_list_executed;
}

// In VertexManager, we don't know the 'real' size of the allocation at the time
// we call AllocateSpaceInBuffer. We have to conservatively allocate 16MB (!).
// After the vertex data is written, we can choose to specify the 'real' allocation
// size to avoid wasting space.
void D3DStreamBuffer::OverrideSizeOfPreviousAllocation(size_t override_allocation_size)
{
  m_buffer_offset = m_buffer_current_allocation_offset + override_allocation_size;
}

void D3DStreamBuffer::AllocateBuffer(size_t size)
{
  // First, put existing buffer (if it exists) in deferred destruction list.
  if (m_buffer)
  {
    D3D12_RANGE write_range = { 0, m_buffer_size };
    m_buffer->Unmap(0, &write_range);
    D3D::command_list_mgr->DestroyResourceAfterCurrentCommandListExecuted(m_buffer.Detach());
  }
  CD3DX12_HEAP_PROPERTIES hprops(D3D12_HEAP_TYPE_UPLOAD);
  auto rdesc = CD3DX12_RESOURCE_DESC::Buffer(size);
  CheckHR(
    D3D::device->CreateCommittedResource(
      &hprops,
      D3D12_HEAP_FLAG_NONE,
      &rdesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
    )
  );

  D3D12_RANGE read_range = {};
  CheckHR(m_buffer->Map(0, &read_range, &m_buffer_cpu_address));

  m_buffer_gpu_address = m_buffer->GetGPUVirtualAddress();
  m_buffer_size = size;

  // Start at the beginning of the new buffer.
  m_buffer_gpu_completion_offset = 0;
  m_buffer_current_allocation_offset = 0;
  m_buffer_offset = 0;

  // Notify observers.
  if (m_buffer_reallocation_notification != nullptr)
    *m_buffer_reallocation_notification = true;

  // If we had any fences queued, they are no longer relevant.
  ClearFences();
}

// Function returns true if current command list executed as a result of current command list
// referencing all of buffer's contents, AND we are already at max_size. No alternative but to
// flush. See comments above AllocateSpaceInBuffer for more details.
bool D3DStreamBuffer::AttemptBufferResizeOrElseStall(size_t allocation_size, bool allow_execute)
{
  // This function will attempt to increase the size of the buffer, in response
  // to running out of room. If the buffer is already at its maximum size specified
  // at creation time, then stall waiting for the GPU to finish with the currently
  // requested memory.

  // Four possibilities, in order of desirability.
  // 1) Best - Update GPU tracking progress - maybe the GPU has made enough
  //    progress such that there is now room.
  // 2) Enlarge GPU buffer, up to our max allowed size.
  // 3) Stall until GPU finishes existing queued work/advances offset
  //    in buffer enough to free room.
  // 4) Worst - flush current GPU commands and wait, which will free all room
  //    in buffer.

  // 1) First, let's check if GPU has already continued farther along buffer. If it has freed up
  // enough of the buffer, we won't have to stall/allocate new memory.

  UpdateGPUProgress();

  // Now that GPU progress is updated, do we have room in the queue?
  if (AttemptToAllocateOutOfExistingUnusedSpaceInBuffer(allocation_size))
  {
    return false;
  }

  // 2) Next, prefer increasing buffer size instead of stalling.
  size_t new_size = std::min(m_buffer_size * 2, m_buffer_max_size);
  new_size = std::max(new_size, allocation_size);

  // Can we grow buffer further?
  if (new_size > m_buffer_size)
  {
    AllocateBuffer(new_size);
    m_buffer_offset = allocation_size;
    return false;
  }

  // 3) Bad case - we need to stall.
  // This might be ok if we have > 2 frames queued up or something, but
  // we don't want to be stalling as we generate the front-of-queue frame.

  const bool found_fence_to_wait_on = AttemptToFindExistingFenceToStallOn(allocation_size);

  if (found_fence_to_wait_on)
  {
    return false;
  }

  // If allow_execute is false, the caller cannot handle command list execution (and the associated reset), so re-allocate the same-sized buffer.
  if (!allow_execute)
  {
    AllocateBuffer(new_size);
    m_buffer_offset = allocation_size;
    return false;
  }

  // 4) If we get to this point, that means there is no outstanding queued GPU work, and we're still out of room.
  // This is bad - and performance will suffer due to the CPU/GPU serialization, but the show must go on.

  // This is guaranteed to succeed, since we've already CHECK'd that the allocation_size <= max_buffer_size, and flushing now and waiting will
  // free all space in buffer.

  D3D::command_list_mgr->ExecuteQueuedWork(true);

  m_buffer_offset = allocation_size;
  m_buffer_current_allocation_offset = 0;
  m_buffer_gpu_completion_offset = 0;

  return true;
}

// Return true if space is found.
bool D3DStreamBuffer::AttemptToAllocateOutOfExistingUnusedSpaceInBuffer(size_t allocation_size)
{
  // First, check if there is room at end of buffer. Fast path.
  if (m_buffer_offset >= m_buffer_gpu_completion_offset)
  {
    if (m_buffer_offset + allocation_size <= m_buffer_size)
    {
      m_buffer_current_allocation_offset = m_buffer_offset;
      m_buffer_offset += allocation_size;
      return true;
    }

    if (allocation_size < m_buffer_gpu_completion_offset)
    {
      m_buffer_current_allocation_offset = 0;
      m_buffer_offset = allocation_size;
      return true;
    }
  }

  // Next, check if there is room at front of buffer. Fast path.
  if (m_buffer_offset < m_buffer_gpu_completion_offset && m_buffer_offset + allocation_size < m_buffer_gpu_completion_offset)
  {
    m_buffer_current_allocation_offset = m_buffer_offset;
    m_buffer_offset += allocation_size;
    return true;
  }

  return false;
}

// Returns true if fence was found and waited on.
bool D3DStreamBuffer::AttemptToFindExistingFenceToStallOn(size_t allocation_size)
{
  // Let's find the first fence that will free up enough space in our buffer.

  UINT64 fence_value_required = 0;

  while (!m_queued_fences.empty())
  {
    FenceTrackingInformation tracking_information = m_queued_fences.front();
    m_queued_fences.pop_front();

    if (m_buffer_offset >= m_buffer_gpu_completion_offset)
    {
      // At this point, we need to wrap around, so req'd gpu offset is allocation_size.
      if (tracking_information.buffer_offset >= allocation_size)
      {
        fence_value_required = tracking_information.fence_value;
        m_buffer_current_allocation_offset = 0;
        m_buffer_offset = allocation_size;
        break;
      }
    }
    else
    {
      if (m_buffer_offset + allocation_size <= m_buffer_size)
      {
        if (tracking_information.buffer_offset >= m_buffer_offset + allocation_size)
        {
          fence_value_required = tracking_information.fence_value;
          m_buffer_current_allocation_offset = m_buffer_offset;
          m_buffer_offset = m_buffer_offset + allocation_size;
          break;
        }
      }
      else
      {
        if (tracking_information.buffer_offset >= allocation_size)
        {
          fence_value_required = tracking_information.fence_value;
          m_buffer_current_allocation_offset = 0;
          m_buffer_offset = allocation_size;
          break;
        }
      }
    }
  }

  // Check if we found a fence we can wait on, for GPU to make sufficient progress.
  // If so, wait on it.
  if (fence_value_required > 0)
  {
    D3D::command_list_mgr->WaitOnCPUForFence(m_buffer_tracking_fence, fence_value_required);
    return true;
  }

  return false;
}

void D3DStreamBuffer::UpdateGPUProgress()
{
  const UINT64 fence_value = m_buffer_tracking_fence->GetCompletedValue();

  while (!m_queued_fences.empty())
  {
    FenceTrackingInformation tracking_information = m_queued_fences.front();
    m_queued_fences.pop_front();

    // Has fence gone past this point?
    if (fence_value >= tracking_information.fence_value)
    {
      m_buffer_gpu_completion_offset = tracking_information.buffer_offset;
    }
    else
    {
      // Fences are stored in assending order, so once we hit a fence we haven't yet crossed on GPU, abort search.
      break;
    }
  }
}

void D3DStreamBuffer::QueueFenceCallback(void* owning_object, UINT64 fence_value)
{
  reinterpret_cast<D3DStreamBuffer*>(owning_object)->QueueFence(fence_value);
}

void D3DStreamBuffer::ClearFences()
{
  m_queued_fences.clear();
}

void D3DStreamBuffer::QueueFence(UINT64 fence_value)
{
  bool add_to_queue = m_queued_fences.empty();
  if (!add_to_queue)
  {
    add_to_queue = m_queued_fences.back().buffer_offset != m_buffer_offset;
  }
  if (add_to_queue)
  {
    FenceTrackingInformation tracking_information = { fence_value , m_buffer_offset };
    m_queued_fences.emplace_back(std::move(tracking_information));
  }
}

}