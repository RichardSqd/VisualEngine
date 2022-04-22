#include "pch.h"
#include "CommandListManager.h"


CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) :
mType(type),
mQueue(nullptr),
mCommandAllocatorPool(type)
{

}

void CommandQueue::Create(ComPtr<ID3D12Device> device) {
	BREAKIFNULL(device);
	ASSERT(!IsReady());
	ASSERT(mCommandAllocatorPool.GetSize() == 0);

	//create command queue 
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = mType;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	BREAKIFFAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mQueue.GetAddressOf())));
	
	//create fence 
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf()));
	mFence->Signal((uint64_t)mType << 56);

	mFenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	ASSERT(mFenceEventHandle != NULL);

	//create allocator pool
	mCommandAllocatorPool.Create(device);
	ASSERT(IsReady());
}

bool CommandQueue::IsReady() {
	return mQueue != nullptr;
}

CommandListManager::CommandListManager() :
	mDevice(nullptr),
	mGraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
	mComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
	mCopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{

}


void CommandListManager::CreateCommandObjects(ComPtr<ID3D12Device> device) {
	if (device == nullptr) PRINTERROR();
	mDevice = device;
	mGraphicsQueue.Create(mDevice);
	mComputeQueue.Create(mDevice);
	mCopyQueue.Create(mDevice);
}



