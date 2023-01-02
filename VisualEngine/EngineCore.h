#pragma once
#include "Graphics.h"
#include "Model.h"

namespace EngineCore {

    class IVisualApp {
    public:
        virtual void InitApp(void) = 0;
        virtual void Update(void) = 0;
        virtual void Draw(void) = 0;
        virtual void Run(void) = 0;
        virtual void ShutDown(void) = 0;
    };

	int StartUp(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nShowCmd);

    bool UpdateApp();

    void ShutDown();

    void InitWindow(_In_ HINSTANCE hInstance, const wchar_t* className);

    IVisualApp* InitAppBasedOnConfig();

    extern IVisualApp* app;
    extern Scene::Model eModel;
}