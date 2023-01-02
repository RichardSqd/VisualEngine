#include "pch.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE mCommandListType):
mCommandListType(mCommandListType),
mDevice(nullptr)
{

}

CommandAllocatorPool::~CommandAllocatorPool() {
	
}

void CommandAllocatorPool::Create(ComPtr<ID3D12Device> device) {
	mDevice = device;
	mAllocatorPool.clear();
}

UINT CommandAllocatorPool::GetSize() {
	return (UINT)mAllocatorPool.size();
}

ComPtr<ID3D12CommandAllocator> CommandAllocatorPool::ArrangeAllocator() {
	//TODO: find available allocators before creating a new one 
	ComPtr<ID3D12CommandAllocator> allocator;
	mDevice->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(allocator.GetAddressOf()));
	mAllocatorPool.push_back(allocator);
	return allocator;
}