#pragma once
#include "pch.h"
#include "CommandListManager.h"




class CommandContext {
	CommandContext() {};


protected:
	ID3D12GraphicsCommandList* commandList;
	ID3D12CommandAllocator* allocator;




};