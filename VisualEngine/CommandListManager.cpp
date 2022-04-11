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
	//ASSERT(mAllocatorPool.Size()=)
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = mType;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	BREAKIFFAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mQueue)));
	//mQueue->SetName
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
	if (device != nullptr) PRINTERROR();
	mDevice = device;
	mGraphicsQueue.Create(mDevice);
	mComputeQueue.Create(mDevice);
	mComputeQueue.Create(mDevice);
}