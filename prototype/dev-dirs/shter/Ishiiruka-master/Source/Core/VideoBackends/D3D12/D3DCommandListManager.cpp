// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <queue>
#include <vector>

#include "VideoBackends/D3D12/D3DBase.h"
#include "VideoBackends/D3D12/D3DCommandListManager.h"
#include "VideoBackends/D3D12/D3DDescriptorHeapManager.h"
#include "VideoBackends/D3D12/D3DQueuedCommandList.h"
#include "VideoBackends/D3D12/D3DState.h"
#include "VideoBackends/D3D12/D3DTexture.h"

#include "VideoBackends/D3D12/Render.h"
#include "VideoBackends/D3D12/ShaderConstantsManager.h"
#include "VideoBackends/D3D12/VertexManager.h"

#include "VideoCommon/VideoConfig.h"

static constexpr unsigned int COMMAND_ALLOCATORS_PER_LIST = 2;

namespace DX12
{
D3DCommandListManager::D3DCommandListManager(
  D3D12_COMMAND_LIST_TYPE command_list_type,
  ID3D12Device* device,
  ID3D12CommandQueue* command_queue
) :
  m_device(device),
  m_command_queue(command_queue)
{
  // Create two lists, with two command allocators each. This corresponds to up to two frames in flight at once.
  m_current_command_allocator = 0;
  m_current_command_allocator_list = 0;
  for (UINT i = 0; i < COMMAND_ALLOCATORS_PER_LIST; i++)
  {
    for (UINT j = 0; j < m_command_allocator_lists.size(); j++)
    {
      ID3D12CommandAllocator* command_allocator = nullptr;

      CheckHR(m_device->CreateCommandAllocator(command_list_type, IID_PPV_ARGS(&command_allocator)));
      m_command_allocator_lists[j].push_back(command_allocator);
    }
  }

  // Create backing command list.
  CheckHR(m_device->CreateCommandList(0, command_list_type, m_command_allocator_lists[m_current_command_allocator_list][0], nullptr, IID_PPV_ARGS(&m_backing_command_list)));

  if (g_ActiveConfig.bBackendMultithreading)
  {
    m_queued_command_list = new ID3D12QueuedCommandList(m_backing_command_list, m_command_queue);
  }

  // Create fence that will be used to measure GPU progress of app rendering requests (e.g. CPU readback of GPU data).
  m_queue_fence_value = 0;
  CheckHR(m_device->CreateFence(m_queue_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_queue_fence)));

  // Create fence that will be used internally by D3DCommandListManager for frame-level resource tracking.
  m_queue_frame_fence_value = 0;
  CheckHR(m_device->CreateFence(m_queue_frame_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_queue_frame_fence)));

  // Create event that will be used for waiting on CPU until a fence is signaled by GPU.
  m_wait_on_cpu_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  // Pre-size the deferred destruction lists.
  for (UINT i = 0; i < m_deferred_destruction_lists.size(); i++)
  {
    m_deferred_destruction_lists[i].reserve(200);
  }

  m_current_deferred_destruction_list = 0;

  std::fill(m_command_allocator_list_fences.begin(), m_command_allocator_list_fences.end(), 0);
  std::fill(m_deferred_destruction_list_fences.begin(), m_deferred_destruction_list_fences.end(), 0);
  m_draws_since_last_execution = 0;
  m_this_frame_draws = 0;
  m_last_frame_draws = 512;
  m_cpu_access_last_frame = false;
  m_cpu_access_this_frame = false;
}

void D3DCommandListManager::SetInitialCommandListState()
{
  ID3D12GraphicsCommandList* command_list = nullptr;
  GetCommandList(&command_list);

  command_list->SetDescriptorHeaps(static_cast<unsigned int>(D3D::gpu_descriptor_heaps.size()), D3D::gpu_descriptor_heaps.data());
  command_list->SetGraphicsRootSignature(D3D::GetRootSignature());

  if (g_renderer)
  {
    // It is possible that we change command lists in the middle of the frame. In that case, restore
    // the viewport/scissor to the current console GPU state.
    g_renderer->RestoreAPIState();
  }

  m_command_list_dirty_state = UINT_MAX;

  command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_command_list_current_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

  if (g_vertex_manager)
    static_cast<VertexManager*>(g_vertex_manager.get())->SetIndexBuffer();
}

void D3DCommandListManager::GetCommandList(ID3D12GraphicsCommandList** command_list) const
{
  *command_list = g_ActiveConfig.bBackendMultithreading ? this->m_queued_command_list : this->m_backing_command_list;
}
void D3DCommandListManager::EnsureDrawLimit()
{
  ++m_draws_since_last_execution;
  ++m_this_frame_draws;
  u32 max_draws = m_last_frame_draws >> (m_cpu_access_last_frame ? 2 : 1);
  max_draws = std::max(max_draws, 64u);
  if (m_draws_since_last_execution > max_draws)
  {
    ExecuteQueuedWork();
  }
}

void D3DCommandListManager::ExecuteQueuedWork(bool wait_for_gpu_completion, bool terminate_worker_tread)
{
  m_queue_fence_value++;

  if (g_ActiveConfig.bBackendMultithreading)
  {
    m_queued_command_list->Close();
    m_queued_command_list->QueueExecute();
    m_queued_command_list->QueueFenceGpuSignal(m_queue_fence, m_queue_fence_value);
    m_queued_command_list->ProcessQueuedItems(wait_for_gpu_completion, wait_for_gpu_completion);
  }
  else
  {
    CheckHR(m_backing_command_list->Close());

    ID3D12CommandList* const execute_list[1] = { m_backing_command_list };
    m_command_queue->ExecuteCommandLists(1, execute_list);

    CheckHR(m_command_queue->Signal(m_queue_fence, m_queue_fence_value));
  }

  // Notify observers of the fence value for the current work to finish.
  for (auto it : m_queue_fence_callbacks)
    it.second(it.first, m_queue_fence_value);

  if (wait_for_gpu_completion || terminate_worker_tread)
    WaitForGPUCompletion(terminate_worker_tread);

  if (!terminate_worker_tread)
  {
    // Re-open the command list, using the current allocator.
    ResetCommandList();
    SetInitialCommandListState();
  }
  m_draws_since_last_execution = 0;
}

void D3DCommandListManager::ExecuteQueuedWorkAndPresent(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags)
{
  m_queue_fence_value++;

  if (g_ActiveConfig.bBackendMultithreading)
  {
    m_queued_command_list->Close();
    m_queued_command_list->QueueExecute();
    m_queued_command_list->QueuePresent(swap_chain, sync_interval, flags);
    m_queued_command_list->QueueFenceGpuSignal(m_queue_fence, m_queue_fence_value);
    m_queued_command_list->ProcessQueuedItems(true);
  }
  else
  {
    CheckHR(m_backing_command_list->Close());

    ID3D12CommandList* const execute_list[1] = { m_backing_command_list };
    m_command_queue->ExecuteCommandLists(1, execute_list);

    CheckHR(swap_chain->Present(sync_interval, flags));
    CheckHR(m_command_queue->Signal(m_queue_fence, m_queue_fence_value));
  }

  // Notify observers of the fence value for the current work to finish.
  for (auto it : m_queue_fence_callbacks)
    it.second(it.first, m_queue_fence_value);

  // Move to the next command allocator, this may mean switching allocator lists.
  MoveToNextCommandAllocator();
  ResetCommandList();
  SetInitialCommandListState();
  m_cpu_access_last_frame = m_cpu_access_this_frame;
  m_last_frame_draws = m_this_frame_draws;
  m_this_frame_draws = 0;
  m_cpu_access_this_frame = false;
  m_draws_since_last_execution = 0;
}

void D3DCommandListManager::DestroyAllPendingResources()
{
  for (auto& destruction_list : m_deferred_destruction_lists)
  {
    for (auto& resource : destruction_list)
      resource->Release();

    destruction_list.clear();
  }
  for (auto& free_list : m_deferred_descriptor_free_lists)
  {
    for (auto& iter : free_list)
      iter.first->Free(iter.second);

    free_list.clear();
  }
}

void D3DCommandListManager::ResetAllCommandAllocators()
{
  for (auto& allocator_list : m_command_allocator_lists)
  {
    for (auto& allocator : allocator_list)
      allocator->Reset();
  }

  // Move back to the start, using the first allocator of first list.
  m_current_command_allocator = 0;
  m_current_command_allocator_list = 0;
  m_current_deferred_destruction_list = 0;
}

void D3DCommandListManager::WaitForGPUCompletion(bool terminate_worker_tread)
{
  // Wait for GPU to finish all outstanding work.
  // This method assumes that no command lists are open.
  m_queue_frame_fence_value++;

  if (g_ActiveConfig.bBackendMultithreading)
  {
    m_queued_command_list->QueueFenceGpuSignal(m_queue_frame_fence, m_queue_frame_fence_value);
    m_queued_command_list->ProcessQueuedItems(true, terminate_worker_tread, terminate_worker_tread);
  }
  else
  {
    CheckHR(m_command_queue->Signal(m_queue_frame_fence, m_queue_frame_fence_value));
  }

  WaitOnCPUForFence(m_queue_frame_fence, m_queue_frame_fence_value);

  // GPU is up to date with us. Therefore, it has finished with any pending resources.
  DestroyAllPendingResources();

  // Command allocators are also up-to-date, so reset these.
  ResetAllCommandAllocators();
}

void D3DCommandListManager::PerformGPURolloverChecks()
{
  m_queue_frame_fence_value++;

  if (g_ActiveConfig.bBackendMultithreading)
  {
    m_queued_command_list->QueueFenceGpuSignal(m_queue_frame_fence, m_queue_frame_fence_value);
  }
  else
  {
    CheckHR(m_command_queue->Signal(m_queue_frame_fence, m_queue_frame_fence_value));
  }
  // We now know that the previous 'set' of command lists has completed on GPU, and it is safe to
  // release resources / start back at beginning of command allocator list.

  // Begin Deferred Resource Destruction
  UINT safe_to_delete_deferred_destruction_list = (m_current_deferred_destruction_list - 1) % m_deferred_destruction_lists.size();
  WaitOnCPUForFence(m_queue_frame_fence, m_deferred_destruction_list_fences[safe_to_delete_deferred_destruction_list]);

  for (UINT i = 0; i < m_deferred_destruction_lists[safe_to_delete_deferred_destruction_list].size(); i++)
  {
    CHECK(m_deferred_destruction_lists[safe_to_delete_deferred_destruction_list][i]->Release() == 0, "Resource leak.");
  }

  m_deferred_destruction_lists[safe_to_delete_deferred_destruction_list].clear();

  for (auto& iter : m_deferred_descriptor_free_lists[safe_to_delete_deferred_destruction_list])
    iter.first->Free(iter.second);
  m_deferred_descriptor_free_lists[safe_to_delete_deferred_destruction_list].clear();

  m_deferred_destruction_list_fences[m_current_deferred_destruction_list] = m_queue_frame_fence_value;
  m_current_deferred_destruction_list = (m_current_deferred_destruction_list + 1) % m_deferred_destruction_lists.size();
  // End Deferred Resource Destruction


  // Begin Command Allocator Resets
  UINT safe_to_reset_command_allocator_list = (m_current_command_allocator_list - 1) % m_command_allocator_lists.size();
  WaitOnCPUForFence(m_queue_frame_fence, m_command_allocator_list_fences[safe_to_reset_command_allocator_list]);

  for (UINT i = 0; i < m_command_allocator_lists[safe_to_reset_command_allocator_list].size(); i++)
  {
    CheckHR(m_command_allocator_lists[safe_to_reset_command_allocator_list][i]->Reset());
  }

  m_command_allocator_list_fences[m_current_command_allocator_list] = m_queue_frame_fence_value;
  m_current_command_allocator_list = (m_current_command_allocator_list + 1) % m_command_allocator_lists.size();
  m_current_command_allocator = 0;
  // End Command Allocator Resets
}

void D3DCommandListManager::MoveToNextCommandAllocator()
{
  // Move to the next allocator in the current allocator list.
  m_current_command_allocator = (m_current_command_allocator + 1) % m_command_allocator_lists[m_current_command_allocator_list].size();

  // Did we wrap around? Move to the next set of allocators.
  if (m_current_command_allocator == 0)
    PerformGPURolloverChecks();
}

void D3DCommandListManager::ResetCommandList()
{
  if (g_ActiveConfig.bBackendMultithreading)
  {
    CheckHR(m_queued_command_list->Reset(m_command_allocator_lists[m_current_command_allocator_list][m_current_command_allocator], nullptr));
  }
  else
  {
    CheckHR(m_backing_command_list->Reset(m_command_allocator_lists[m_current_command_allocator_list][m_current_command_allocator], nullptr));
  }
}

void D3DCommandListManager::DestroyResourceAfterCurrentCommandListExecuted(ID3D12Resource* resource)
{
  CHECK(resource, "Null resource being inserted!");

  m_deferred_destruction_lists[m_current_deferred_destruction_list].push_back(resource);
}

void D3DCommandListManager::FreeDescriptorAfterCurrentCommandListExecuted(D3DDescriptorHeapManager* descriptor_heap,
  size_t index)
{
  m_deferred_descriptor_free_lists[m_current_deferred_destruction_list].emplace_back(descriptor_heap, index);
}

D3DCommandListManager::~D3DCommandListManager()
{
  if (g_ActiveConfig.bBackendMultithreading)
  {
    // Wait for background thread to exit.
    m_queued_command_list->Release();
  }
  else
  {
    // The command list will still be open, close it before destroying.
    m_backing_command_list->Close();
  }
  // Descriptor heaps are already freed at this point, as they depend on us.
  // Just throw away the descriptors.
  for (auto& free_list : m_deferred_descriptor_free_lists)
    free_list.clear();

  DestroyAllPendingResources();

  m_backing_command_list->Release();

  for (auto& allocator_list : m_command_allocator_lists)
  {
    for (auto& resource : allocator_list)
      resource->Release();
  }

  m_queue_fence->Release();
  m_queue_frame_fence->Release();

  CloseHandle(m_wait_on_cpu_fence_event);
}

void D3DCommandListManager::WaitOnCPUForFence(ID3D12Fence* fence, UINT64 fence_value)
{
  if (fence->GetCompletedValue() >= fence_value)
    return;

  CheckHR(fence->SetEventOnCompletion(fence_value, m_wait_on_cpu_fence_event));
  WaitForSingleObject(m_wait_on_cpu_fence_event, INFINITE);
}

void D3DCommandListManager::SetCommandListDirtyState(unsigned int command_list_state, bool dirty)
{
  if (dirty)
    m_command_list_dirty_state |= command_list_state;
  else
    m_command_list_dirty_state &= ~command_list_state;
}

bool D3DCommandListManager::GetCommandListDirtyState(COMMAND_LIST_STATE command_list_state) const
{
  return ((m_command_list_dirty_state & command_list_state) != 0);
}

void D3DCommandListManager::SetCommandListPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitive_topology)
{
  m_command_list_current_topology = primitive_topology;
}

D3D_PRIMITIVE_TOPOLOGY D3DCommandListManager::GetCommandListPrimitiveTopology() const
{
  return m_command_list_current_topology;
}

void D3DCommandListManager::CPUAccessNotify()
{
  m_cpu_access_last_frame = true;
  m_cpu_access_this_frame = true;
  m_draws_since_last_execution = 0;
};

ID3D12Fence* D3DCommandListManager::RegisterQueueFenceCallback(void* owning_object, PFN_QUEUE_FENCE_CALLBACK* callback_function)
{
  m_queue_fence_callbacks[owning_object] = callback_function;

  return m_queue_fence;
}

void D3DCommandListManager::RemoveQueueFenceCallback(void* owning_object)
{
  m_queue_fence_callbacks.erase(owning_object);
}

}  // namespace DX12
