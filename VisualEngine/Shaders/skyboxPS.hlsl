
#include "Common.hlsli"

SamplerState samPointWrap	 : register(s0);
SamplerState samPointClamp  : register(s1);
SamplerState samLinearWrap	 : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAnisotropicWrap : register(s4);
SamplerState samAnisotropicClamp : register(s5);

TextureCube<float3> cubeMap : register(t0);

cbuffer cbGlobal : register(b0)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 CameraPos;
	float NearZ;
	int shaderSelector;
	float FarZ;
}



struct pixelIn
{
	float4 position  : SV_POSITION;
	float3 viewDir: TEXCOORD3;

	
};


float4 PS(pixelIn pin) : SV_Target
{
	return float4( cubeMap.Sample(samLinearWrap, pin.viewDir), 1);  
}

