#pragma once
namespace EngineCore {

	int StartUp(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nShowCmd);

    void ShutDown();

}