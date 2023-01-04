#pragma once
#include "pch.h"
#include "CommandListManager.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
namespace Graphics {

	extern uint32_t gWidth;
	extern uint32_t gHeight;
	extern HWND ghWnd;
	extern ComPtr<ID3D12Device> gDevice;
	extern ComPtr<IDXGISwapChain3> gSwapChain;
	extern ComPtr<ID3D12DescriptorHeap> gRtvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gDsvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gCbvSrvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gSamplerHeap;
	extern CommandQueueManager gCommandQueueManager;
	extern CommandContextManager gCommandContextManager;
	extern FrameResourceManager gFrameResourceManager;
	extern UINT gRTVDescriptorSize;
	extern UINT gDSVDescriptorSize;
	extern UINT gCbvSrvUavDescriptorSize;
	extern UINT gSamplerDescriptorSize;
	extern UINT gNumFrameResources;
	extern UINT gNumFrameBuffers;
	extern UINT gFrameIndex;
	extern std::vector<std::unique_ptr<FrameResource>> gFrameResources;

	void Init(bool EnableDXR);
	void CreateSwapChain();
	void CreateDescriptorHeaps();

	

}