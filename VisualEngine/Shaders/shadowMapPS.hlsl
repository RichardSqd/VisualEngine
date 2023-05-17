
#include "Common.hlsli"

SamplerState samPointWrap	 : register(s0);
SamplerState samPointClamp  : register(s1);
SamplerState samLinearWrap	 : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAnisotropicWrap : register(s4);
SamplerState samAnisotropicClamp : register(s5);


Texture2D diffuseMap : register(t1);
Texture2D metallicRoughnessMap : register(t2);
Texture2D normalMap : register(t3);
Texture2D occlusionMap: register(t4);
Texture2D emissiveMap: register(t5);


cbuffer cbGlobal : register(b1)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 CameraPos;

}




cbuffer cbLight: register(b3)
{
	SceneLighting lights;
}



struct PixelIn
{
	float4 position  : SV_POSITION;
	float4 tangent: TANGENT;
	float4 positionWorld: POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD;
	
};


float4 PS(PixelIn pin) : SV_Target
{
	return float4(1.0, 0.0, 0.0, 0.0);
}

