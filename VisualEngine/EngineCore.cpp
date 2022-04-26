#include "pch.h"
#include "EngineCore.h"
#include "AVisualApp.h"

namespace EngineCore {
    using namespace DirectX;
    using namespace Graphics;

    IVisualApp* app;

    LRESULT CALLBACK
    WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_ACTIVATE:

            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);

        }

        return 0;

    }

    int StartUp(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nShowCmd) {
        //TODO: run based on config 
        
        if (!XMVerifyCPUSupport())
            return 1;
        //Windows::Foundation::Initialize();
        //Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
        
        InitWindow(hInstance, L"Wnd");
        app = InitAppBasedOnConfig();
        if (!app) {
            PRINTERROR();
            __debugbreak();
        }
        app->InitApp();
        ShowWindow(ghWnd, SW_SHOW);
        //app->Run();
        app->ShutDown();

        return 0;
    }

    void ShutDown() {

    }

    IVisualApp* InitAppBasedOnConfig() {
        //if (Config::RAYTRACE) {
            
        //}
        return new AVisualApp();
    }

    void InitWindow(_In_ HINSTANCE hInstance, const wchar_t* className){
        WNDCLASS wc;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = className;

        if (!RegisterClass(&wc)) {
            Utils::Print("Failed to register main window\n");
            __debugbreak();
        }

        RECT rect = { 0, 0, (LONG)gWidth, (LONG)gHeight };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

        int width = rect.right - rect.left;
        int height = rect.top - rect.bottom;

        ghWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            width, height, nullptr, nullptr, hInstance, nullptr);

        if (!ghWnd) {
            Utils::Print("Failed to create window\n");
            __debugbreak();
        }



        
        
        
        //UpdateWindow(ghWnd);
    }

    bool UpdateApp() {
        


        return 1;
    }

    


}