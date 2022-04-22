#pragma once
#include <queue>
#include <mutex>


using Microsoft::WRL::ComPtr;
class CommandAllocatorPool {
public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE CommandListType);
	~CommandAllocatorPool();

	ComPtr<ID3D12CommandAllocator> ArrangeAllocator();
	void Create(ComPtr<ID3D12Device> device);

	UINT GetSize();

private:
	const D3D12_COMMAND_LIST_TYPE mCommandListType;
	ComPtr<ID3D12Device> mDevice;
	std::vector< ComPtr<ID3D12CommandAllocator>> mAllocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> mAvailableAllocators;
	//std::mutex mAllocatorMutex;
};