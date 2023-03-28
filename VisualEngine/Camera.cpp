#include "pch.h"
#include "Camera.h"


void Camera::UpdateViewMatrix() 
{
	if (viewDirty) {
		viewDirty = false;
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(camPos.x, camPos.y, camPos.z, 1.0f);
		DirectX::XMVECTOR lookat = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMMATRIX xmview = DirectX::XMMatrixLookAtRH(pos, lookat, up);
		DirectX::XMStoreFloat4x4(&view, xmview);
	}
	
}


void Camera::SetCamPosSph(float phi, float theta, float rad) {
	camPos.x = camRadius * sinf(camPhi) * cosf(camTheta);
	camPos.y = camRadius * cosf(camPhi);
	camPos.z = camRadius * sinf(camPhi) * sinf(camTheta);
}

