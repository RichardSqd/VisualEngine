
#include "Common.hlsli"

SamplerState samPointWrap	 : register(s0);
SamplerState samPointClamp  : register(s1);
SamplerState samLinearWrap	 : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAnisotropicWrap : register(s4);
SamplerState samAnisotropicClamp : register(s5);


Texture2D iblTexutreHDR : register(t0);



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
	//float4 tangent: TANGENT;
	float4 positionWorld: POSITION;
	//float3 normal : NORMAL;
	//float2 uv: TEXCOORD;
	
};


float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= float2(0.1591, 0.3183);
	uv += 0.5;
	return uv;
}


float4 PS(pixelIn pin) : SV_Target
{

	float2 uv = SampleSphericalMap(normalize(pin.positionWorld.xyz));
	float3 color = iblTexutreHDR.Sample(samAnisotropicWrap, uv).xyz;
	color = color / (color + float3(1.0, 1.0, 1.0));
	color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
	
	return float4(color, 1.0);

}

