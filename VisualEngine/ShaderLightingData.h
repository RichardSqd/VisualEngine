
#ifndef VE_GPU
#define VE_CPU 1 
#endif // !VE_GPU

#ifdef VE_CPU
#pragma once
#include "pch.h"
#define float3   DirectX::XMFLOAT3
#define float2   DirectX::XMFLOAT2
//#define matrix   DirectX::XMMATRIX
#define float3x3 DirectX::XMFLOAT3X3

namespace ShaderLightingData {
#endif // VE_CPU

#define NUM_POINT_LIGHTS  72
#define NUM_SPOT_LIGHTS  36
#define NUM_DIRECTIONAL_LIGHTS  12

	struct DirectionalLight {
		float3 lightDirection;
		float dummy0;
		float3 color;
		float dummy1;
		float3 strength;
		int enabled;
	};


	struct SpotLight
	{
		float3 lightPosition;
		float RangeStart; //point and spot lights
		float3 lightDirection;
		float RangeEnd; //point and spot lights
		float3 color;
		float dummy0;
		float3 strength;
		int enabled;
	};

	struct PointLight
	{
		float3 lightPosition;
		float RangeStart; //point and spot lights
		float3 color;
		float RangeEnd; //point and spot lights
	};

	struct SceneLighting
	{
		int numDirectionalLights ;
		int numSpotLights;
		int numPointLights;
		float dummy0;

		DirectionalLight directionalLights[NUM_DIRECTIONAL_LIGHTS];
		SpotLight spotLights[NUM_SPOT_LIGHTS];
		PointLight pointLights[NUM_POINT_LIGHTS];
	};

#ifdef VE_CPU
}
#endif // VE_CPU