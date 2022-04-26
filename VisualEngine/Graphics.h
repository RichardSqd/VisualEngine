#pragma once
#include "pch.h"

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
	extern UINT gNumFrame;
	extern UINT gFrameIndex;
	void Init(bool EnableDXR);
	void CreateSwapChain();
	void CreateDescriptorHeaps();

}