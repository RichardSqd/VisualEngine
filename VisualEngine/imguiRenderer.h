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
}