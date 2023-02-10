#pragma once
#include "pch.h"
using namespace DirectX::SimpleMath;
#define MAXLIGHTS 16

struct Light 
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 Direction = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
};