#pragma once
#include "Graphics.h"
namespace EngineCore {

    class IVisualApp {
    public:
        virtual void InitApp(void) = 0;
        virtual void Update(void) = 0;
        virtual void ShutDown(void) = 0;
    };

	int StartUp(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nShowCmd);

    void ShutDown();

    void InitWindow(_In_ HINSTANCE hInstance, const wchar_t* className);

    IVisualApp* InitAppBasedOnConfig();

    extern IVisualApp* app;
}