#include "pch.h"
#include "Config.h"
#include "EngineCore.h"

using namespace EngineCore;

int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    
    int result = EngineCore::StartUp(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    EngineCore::ShutDown();
	return result;
} 