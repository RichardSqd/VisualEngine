#pragma once
#include "pch.h"
#include "CommandAllocatorPool.h"

using Microsoft::WRL::ComPtr;

class CommandQueue {

public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	~CommandQueue() {};

	void Create(ComPtr<ID3D12Device> device);
	bool IsReady();
	ComPtr<ID3D12CommandQueue> GetQueue();
	UINT64 GetCompletedFenceValue();
	ComPtr<ID3D12CommandAllocator> RequestAllocator();
	void FlushCommandQueue();
	void SetEventOnCompletion(UINT64 value, HANDLE eventHandle);
	void AdvanceFenceValue();
	void SignalFencePoint();
	void ExecuteCommandLists(UINT count, ID3D12CommandList** cmdLists);

private:
	ComPtr<ID3D12CommandQueue> mQueue;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurentFence;
	const D3D12_COMMAND_LIST_TYPE mType;
	CommandAllocatorPool mCommandAllocatorPool;
	HANDLE mFenceEventHandle;
};

class CommandQueueManager {
	
public:
	CommandQueueManager() ;
	~CommandQueueManager() {};
	void CreateCommandObjects(ComPtr<ID3D12Device> mDevice);
	

	CommandQueue& GetGraphicsQueue() { return mGraphicsQueue; }
	CommandQueue& GetComputeQueue() { return mComputeQueue; }
	CommandQueue& GetCopyQueue() { return mCopyQueue; }

private:
	ComPtr<ID3D12Device> mDevice;
	CommandQueue mGraphicsQueue;
	CommandQueue mCopyQueue;
	CommandQueue mComputeQueue;

};

class CommandContext {
public:
	CommandContext() {};
	CommandContext(ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12CommandAllocator> commandAllocator)
		: mCommandList(commandList),
		mCommandAllocator(commandAllocator) {}
	ComPtr<ID3D12GraphicsCommandList> getCommandList();
	ComPtr<ID3D12CommandAllocator> getCommandAllocator();

protected:
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;




};

class CommandContextManager {
public:
	CommandContextManager() {}
	~CommandContextManager();
	void CreateCommandContext(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type);
	std::shared_ptr<CommandContext> AllocateContext(D3D12_COMMAND_LIST_TYPE type);

private:
	ComPtr<ID3D12Device> mDevice;
	std::vector<std::shared_ptr<CommandContext>> mContextPool;
	
	

};





class GraphicContext : public CommandContext {
public:

};