// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <map>
#include <vector>

#include "D3DQueuedCommandList.h"

namespace DX12
{
enum COMMAND_LIST_STATE
{
  COMMAND_LIST_STATE_GS_CBV = 1,
  COMMAND_LIST_STATE_HDS_CBV = 2,
  COMMAND_LIST_STATE_PS_CBV = 4,
  COMMAND_LIST_STATE_VS_CBV = 8,
  COMMAND_LIST_STATE_PSO = 16,
  COMMAND_LIST_STATE_SAMPLERS = 32,
  COMMAND_LIST_STATE_VERTEX_BUFFER = 64
};

class D3DDescriptorHeapManager;

// This class provides an abstraction for D3D12 descriptor heaps.
class D3DCommandListManager
{
public:

  D3DCommandListManager(D3D12_COMMAND_LIST_TYPE command_list_type, ID3D12Device* device, ID3D12CommandQueue* command_queue);
  ~D3DCommandListManager();

  void SetInitialCommandListState();

  void GetCommandList(ID3D12GraphicsCommandList** command_list) const;
  void EnsureDrawLimit();
  void ExecuteQueuedWork(bool wait_for_gpu_completion = false, bool terminate_worker_tread = false);
  void ExecuteQueuedWorkAndPresent(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags);

  void DestroyResourceAfterCurrentCommandListExecuted(ID3D12Resource* resource);
  void FreeDescriptorAfterCurrentCommandListExecuted(D3DDescriptorHeapManager* descriptor_heap,
    size_t index);

  void SetCommandListDirtyState(unsigned int command_list_state, bool dirty);
  bool GetCommandListDirtyState(COMMAND_LIST_STATE command_list_state) const;

  void SetCommandListPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitive_topology);
  D3D_PRIMITIVE_TOPOLOGY GetCommandListPrimitiveTopology() const;



  void CPUAccessNotify();

  // Allow other components to register for a callback each time a fence is queued.
  using PFN_QUEUE_FENCE_CALLBACK = void(void* owning_object, UINT64 fence_value);
  ID3D12Fence* RegisterQueueFenceCallback(void* owning_object, PFN_QUEUE_FENCE_CALLBACK* callback_function);
  void RemoveQueueFenceCallback(void* owning_object);

  void WaitOnCPUForFence(ID3D12Fence* fence, UINT64 fence_value);

private:

  void DestroyAllPendingResources();
  void ResetAllCommandAllocators();
  void WaitForGPUCompletion(bool terminate_worker_tread = false);

  void PerformGPURolloverChecks();
  void MoveToNextCommandAllocator();
  void ResetCommandList();

  using PendingDescriptorFree = std::pair<D3DDescriptorHeapManager*, size_t>;

  unsigned int m_draws_since_last_execution = 0;
  unsigned int m_this_frame_draws = 0;
  unsigned int m_last_frame_draws = 0;
  bool m_cpu_access_last_frame = false;
  bool m_cpu_access_this_frame = false;
  unsigned int m_command_list_dirty_state = UINT_MAX;
  D3D_PRIMITIVE_TOPOLOGY m_command_list_current_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

  HANDLE m_wait_on_cpu_fence_event{};

  ID3D12Device* m_device{};
  ID3D12CommandQueue* m_command_queue{};
  UINT64 m_queue_fence_value{};
  ID3D12Fence* m_queue_fence{};
  UINT64 m_queue_frame_fence_value{};
  ID3D12Fence* m_queue_frame_fence{};

  std::map<void*, PFN_QUEUE_FENCE_CALLBACK*> m_queue_fence_callbacks;

  UINT m_current_command_allocator{};
  UINT m_current_command_allocator_list{};
  std::array<std::vector<ID3D12CommandAllocator*>, 2> m_command_allocator_lists;
  std::array<UINT64, 2> m_command_allocator_list_fences{};

  ID3D12GraphicsCommandList* m_backing_command_list{};
  ID3D12QueuedCommandList* m_queued_command_list{};

  UINT m_current_deferred_destruction_list{};
  std::array<std::vector<ID3D12Resource*>, 2> m_deferred_destruction_lists;
  std::array<std::vector<PendingDescriptorFree>, 2> m_deferred_descriptor_free_lists;
  std::array<UINT64, 2> m_deferred_destruction_list_fences;
};

}  // namespace