#pragma once
#include "pch.h"

namespace Control {
	extern std::unique_ptr<DirectX::Keyboard> m_keyboard;
	extern std::unique_ptr<DirectX::Mouse> m_mouse;
	//extern std::unique_ptr<DX::DeviceResources> m_deviceResource;
	extern const float ROTATION_GAIN;
	extern const float MOVEMENT_GAIN;
	extern POINT lastMousePos;
	extern void InitControl(HWND hwnd);
	extern void OnMouseDown(WPARAM btnState, int x, int y);
	extern void OnMouseUp(WPARAM btnState, int x, int y);
	extern void OnMouseMove(WPARAM btnState, int x, int y);
	extern void OnKeyDown(WPARAM btnState);
}