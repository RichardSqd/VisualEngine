
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
	//float2 uv: TEXCOORD;
	
};


float4 PS(pixelIn pin) : SV_Target
{
	//return float4(cubeMap.SampleLevel(samAnisotropicWrap, pin.viewDir, 0), 1);
	//return float4(0.3, 0.0, 1, 1);
	return float4( cubeMap.Sample(samLinearWrap, pin.viewDir), 1);  //float4(0.3,0.0,0.0,0.5);// cubeMap.Sample(samAnisotropicWrap, pin.viewDir);
	//return float4(cubeMap.SampleLevel(samPointWrap, pin.viewDir, 0), 1);
	//return panoramaToCubeMap()

}

