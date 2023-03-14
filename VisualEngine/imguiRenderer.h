#pragma once
#include "pch.h"

#if defined(DEBUG) || defined(_DEBUG)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

namespace imguiRenderer {

    struct FrameContext
    {
        ID3D12CommandAllocator* CommandAllocator;
        UINT64                  FenceValue;
    };


    extern bool Init();
    extern bool CreateDeviceD3D(HWND hWnd);
    extern void CleanupDeviceD3D();
    extern void CreateRenderTarget();
    extern void CleanupRenderTarget();
    extern void WaitForLastSubmittedFrame();
    extern FrameContext* WaitForNextFrameResources();
    extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    extern void ImGui_NewFrame();
}