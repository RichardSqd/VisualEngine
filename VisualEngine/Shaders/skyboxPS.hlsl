
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

/*
float3 uvToXYZ(int face, float2 uv)
{
	if (face == 0)
		return float3(1.f, uv.y, -uv.x);

	else if (face == 1)
		return float3(-1.f, uv.y, uv.x);

	else if (face == 2)
		return float3(+uv.x, -1.f, +uv.y);

	else if (face == 3)
		return float3(+uv.x, 1.f, -uv.y);

	else if (face == 4)
		return float3(+uv.x, uv.y, 1.f);

	else //if(face == 5)
	{
		return float3(-uv.x, +uv.y, -1.f);
	}
}

float2 dirToUV(float3 dir)
{
	return float2(
		0.5f + 0.5f * atan2(dir.z, dir.x) / 3.1415926535,
		1.f - acos(dir.y) / 3.1415926535);
}

float3 panoramaToCubeMap(int face, float2 texCoord)
{
	float2 texCoordNew = texCoord * 2.0 - 1.0; //< mapping vom 0,1 to -1,1 coords
	float3 scan = uvToXYZ(face, texCoordNew);
	float3 direction = normalize(scan);
	float2 src = dirToUV(direction);

	return  cubeMap.Sample(samPointWrap, src).rgb; //< get the color from the panorama
}
*/
float4 PS(pixelIn pin) : SV_Target
{
	//return float4(cubeMap.SampleLevel(samAnisotropicWrap, pin.viewDir, 0), 1);
	//return float4(0.3, 0.0, 1, 1);
	return float4( cubeMap.Sample(samLinearWrap, pin.viewDir), 1);  //float4(0.3,0.0,0.0,0.5);// cubeMap.Sample(samAnisotropicWrap, pin.viewDir);
	//return float4(cubeMap.SampleLevel(samPointWrap, pin.viewDir, 0), 1);
	//return panoramaToCubeMap()

}

