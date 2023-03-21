#include "pch.h"
#include "Control.h"
#include "Graphics.h"
#include "Renderer.h"

using namespace DirectX;

namespace Control {
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;
    bool cameraLocked = false;
	const float ROTATION_GAIN = 0.004f;
	const float MOVEMENT_GAIN = 0.07f;
	POINT lastMousePos {};
    
	void InitControl(HWND hwnd) {

		m_keyboard = std::make_unique<Keyboard>();
		m_mouse = std::make_unique<Mouse>();
		m_mouse->SetWindow(hwnd);

	}

	void OnMouseDown(WPARAM btnState, int x, int y) {
        if (!cameraLocked) {
            lastMousePos.x = x;
            lastMousePos.y = y;
            SetCapture(Graphics::ghWnd);
        }
		

	}
	void OnMouseUp(WPARAM btnState, int x, int y) {
        
        if (!cameraLocked) {
            ReleaseCapture();
        }
	}
	void OnMouseMove(WPARAM btnState, int x, int y) {
        if (!cameraLocked) {
            if ((btnState & MK_LBUTTON) != 0)
            {
                // Make each pixel correspond to a quarter of a degree.
                float dx = XMConvertToRadians(0.25f * static_cast<float>(x - lastMousePos.x));
                float dy = XMConvertToRadians(0.25f * static_cast<float>(y - lastMousePos.y));

                // Update angles based on input to orbit camera around box.
                Renderer::gMainCam.camTheta += dx;
                Renderer::gMainCam.camPhi += dy;

                // Restrict the angle mPhi.
                Renderer::gMainCam.camPhi = Math::Clamp(Renderer::gMainCam.camPhi, 0.1f, Math::PI - 0.1f);
                Renderer::gMainCam.viewDirty = true;
            }
            else if ((btnState & MK_RBUTTON) != 0)
            {
                // Make each pixel correspond to 0.005 unit in the scene.
                float dx = 0.005f * static_cast<float>(x - lastMousePos.x);
                float dy = 0.005f * static_cast<float>(y - lastMousePos.y);

                // Update the camera radius based on input.
                Renderer::gMainCam.camRadius += dx - dy;
                Renderer::gMainCam.viewDirty = true;
                // Restrict the radius.
                //Renderer::gMainCam.camRadius = Math::Clamp(Renderer::gMainCam.camRadius, 3.0f, 150.0f);
            }

            lastMousePos.x = x;
            lastMousePos.y = y;
        }
	}

    void OnKeyDown(WPARAM btnState) {
        if (static_cast<UINT8>(btnState) == VK_ESCAPE)
            PostQuitMessage(0);
        else if (static_cast<UINT8>(btnState) == VK_SPACE && Graphics::gRayTraceEnvironmentActive) {
            Graphics::gRayTraced = !Graphics::gRayTraced;
        }
        else if (static_cast<UINT8>(btnState) == 'L') {      
            cameraLocked = !cameraLocked;
        }
        


    }
}