#pragma once
#include "pch.h"
#include "CommandListManager.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
namespace Graphics {
	extern bool gRayTraceEnvironmentActive;
	extern bool gRayTraced;
	extern float gWidth;
	extern float gHeight;
	extern HWND ghWnd;
	extern ComPtr<ID3D12Device> gDevice;
	extern ComPtr<ID3D12Device5> gDXRDevice;
	extern ComPtr<IDXGISwapChain3> gSwapChain;
	extern ComPtr<ID3D12DescriptorHeap> gRtvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gDsvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gCbvSrvHeap;
	extern ComPtr<ID3D12DescriptorHeap> gSamplerHeap;
	extern ComPtr<IDXGIFactory6> gdxgiFactory;
	extern CommandQueueManager gCommandQueueManager;
	extern CommandContextManager gCommandContextManager;
	extern FrameResourceManager gFrameResourceManager;

	extern UINT gSwapChainBufferCount;
	extern UINT gRTVDescriptorSize;
	extern UINT gDSVDescriptorSize;
	extern UINT gCbvSrvUavDescriptorSize;
	extern UINT gSamplerDescriptorSize;
	extern UINT gNumFrameResources;
	extern UINT gFrameIndex;
	extern UINT gPassCbvOffset;
	extern UINT gObjectCBByteSize;
	extern UINT gPassCBByteSize;
	extern UINT gMatCBByteSize;
	extern std::vector<std::unique_ptr<FrameResource>> gFrameResources;

	extern D3D12_VIEWPORT gScreenViewport;
	extern D3D12_RECT gScissorRect;

	extern DXGI_FORMAT gBackBufferFormat;
	extern DXGI_FORMAT gDepthStencilFormat;

	void Init();
	extern float AspectRatio();
	extern void CheckDXRSupport(bool& rayTraceStatus);

	

}