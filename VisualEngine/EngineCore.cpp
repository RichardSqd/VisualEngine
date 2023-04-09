#include "pch.h"
#include "EngineCore.h"
#include "AVisualApp.h"
#include "Renderer.h"
#include <WindowsX.h>
#include "Control.h"
#include "imgui_impl_win32.h"

namespace EngineCore {
    using namespace DirectX;
    using namespace Graphics;

    IVisualApp* app = nullptr;
    Scene::Model eModel;

    LRESULT CALLBACK
    WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;
        switch (msg)
        {

        case WM_ACTIVATEAPP:
            //Keyboard::ProcessMessage(msg, wParam, lParam);
            //Mouse::ProcessMessage(msg, wParam, lParam);
            break;

        case WM_SIZE:
            Graphics::gWidth = LOWORD(lParam);
            Graphics::gHeight = HIWORD(lParam);
            break;

        case WM_EXITSIZEMOVE:
            Renderer::OnResize();
            break;
            /*
        case WM_ACTIVATE:
        case WM_INPUT:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
        */
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            //Control::OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            //Control::OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        case WM_MOUSEMOVE:
            Control::OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_KEYDOWN:
            Control::OnKeyDown(wParam);
            
                
                
                break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
            //Keyboard::ProcessMessage(msg, wParam, lParam);
            break;

        case WM_DESTROY:
            //__debugbreak();
            PostQuitMessage(0);
            break;

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
        Windows::Foundation::Initialize();
        Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
        
        InitWindow(hInstance, L"Wnd");
        app = InitAppBasedOnConfig();
        if (!app) {
            PRINTERROR();
            __debugbreak();
        }
        app->InitApp();
        ShowWindow(ghWnd, SW_SHOW);
        app->Run();
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
        WNDCLASS wc = {};
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
        int height = rect.bottom - rect.top;

        ghWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            width, height, nullptr, nullptr, hInstance, nullptr);

        if (!ghWnd) {
            Utils::Print("Failed to create window\n");
            __debugbreak();
        }


        //ShowWindow(ghWnd, SW_SHOW);       
        //UpdateWindow(ghWnd);
    }

    bool UpdateApp() {

        app->Update();
        app->Draw();

        //update user input 
       


        //detect if user end the program 


        return 1;
    }

    


}