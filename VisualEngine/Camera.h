#pragma once
#include "pch.h"

struct Camera {
	DirectX::XMFLOAT4X4 proj;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT3 camPos;
	FLOAT camPhi;
	FLOAT camTheta;
	FLOAT camRadius;

};