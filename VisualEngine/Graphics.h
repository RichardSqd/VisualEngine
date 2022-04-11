#pragma once
#include "pch.h"
using Microsoft::WRL::ComPtr;
namespace Graphics {

	extern uint32_t gWidth;
	extern uint32_t gHeight;
	extern HWND ghWnd;
	extern ComPtr<ID3D12Device> gDevice;
	void Init(bool EnableDXR);
}