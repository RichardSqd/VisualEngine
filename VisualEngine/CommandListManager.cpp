#include "pch.h"
#include "CommandListManager.h"
#include "Graphics.h"

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

void CommandQueue::FlushCommandQueue() {
	mCurentFence++;
	BREAKIFFAILED(mQueue->Signal(mFence.Get(), mCurentFence));
	if (mFence->GetCompletedValue() < mCurentFence) {

		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ASSERT(eventHandle)

		BREAKIFFAILED(mFence->SetEventOnCompletion(mCurentFence, eventHandle));
		
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void CommandQueue::SetEventOnCompletion(UINT64 value, HANDLE eventHandle)
{
	BREAKIFFAILED(mFence->SetEventOnCompletion(value,eventHandle));
}

void CommandQueue::AdvanceFenceValue() {
	mCurentFence += 1;
}

void CommandQueue::SignalFencePoint() {
	BREAKIFFAILED(mQueue->Signal(mFence.Get(), mCurentFence));	
}

bool CommandQueue::IsReady() {
	return mQueue != nullptr;
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetQueue() {
	ASSERT(IsReady());
	return mQueue;
}

UINT64 CommandQueue::GetCompletedFenceValue()
{
	return mFence->GetCompletedValue();
}

ComPtr<ID3D12CommandAllocator> CommandQueue::RequestAllocator() {
	//TODO: find available allocators before creating a new one 
	return mCommandAllocatorPool.ArrangeAllocator();
}


CommandQueueManager::CommandQueueManager() :
	mDevice(nullptr),
	mGraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
	mComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
	mCopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{

}


void CommandQueueManager::CreateCommandObjects(ComPtr<ID3D12Device> device) {
	if (device == nullptr) PRINTERROR();
	mDevice = device;
	mGraphicsQueue.Create(mDevice);
	mComputeQueue.Create(mDevice);
	mCopyQueue.Create(mDevice);
}



//-----------------------------------------------------------

std::shared_ptr<CommandContext> CommandContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type) {
	std::shared_ptr<CommandContext> context;
	if (mContextPool.size() == 0) {
		CreateCommandContext(mDevice, type);
	}

	context = mContextPool.back();
	mContextPool.pop_back();
	
	return context;
}

void CommandContextManager::CreateCommandContext(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type) {
	if(device==nullptr) PRINTERROR();
	mDevice = device;

	ComPtr<ID3D12GraphicsCommandList> commandList;
	
	//TODO: For now the command queue manager creates a new allocator each time
	ComPtr<ID3D12CommandAllocator> allocator = Graphics::gCommandQueueManager.GetGraphicsQueue().RequestAllocator();
	BREAKIFFAILED(mDevice->CreateCommandList(0,
		type,
		allocator.Get(),
		nullptr,
		IID_PPV_ARGS(commandList.GetAddressOf())));

	std::shared_ptr<CommandContext> cc(new CommandContext(commandList, allocator));
	mContextPool.push_back(cc);



}

CommandContextManager::~CommandContextManager() {
	
	for (auto it = mContextPool.begin(); it != mContextPool.end(); ++it) {
		(* it).reset();
	}
}


//---------------------------------------------------------------

ComPtr<ID3D12GraphicsCommandList> CommandContext::getCommandList() {
	return mCommandList;
}

ComPtr<ID3D12CommandAllocator> CommandContext::getCommandAllocator() {
	return mCommandAllocator;
}