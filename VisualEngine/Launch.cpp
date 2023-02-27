#include "pch.h"
#include "Config.h"
#include "EngineCore.h"

using namespace EngineCore;

int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        // error
        return 1;
#else
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        // error
        return 1;
#endif
    int result = EngineCore::StartUp(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    EngineCore::ShutDown();
	return 0;
} 