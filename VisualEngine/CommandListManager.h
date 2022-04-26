#pragma once
#include "Graphics.h"
#include "CommandAllocatorPool.h"

using Microsoft::WRL::ComPtr;

class CommandQueue {

public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	~CommandQueue() {};

	void Create(ComPtr<ID3D12Device> device);
	bool IsReady();
	ComPtr<ID3D12CommandQueue> GetQueue();

private:
	ComPtr<ID3D12CommandQueue> mQueue;
	const D3D12_COMMAND_LIST_TYPE mType;
	CommandAllocatorPool mCommandAllocatorPool;
};

class CommandListManager {
	
public:
	CommandListManager() ;
	~CommandListManager() {};
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