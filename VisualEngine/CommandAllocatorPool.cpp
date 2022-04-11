#include "pch.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE mCommandListType):
mCommandListType(mCommandListType),
mDevice(nullptr)
{

}

CommandAllocatorPool::~CommandAllocatorPool() {

}