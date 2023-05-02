
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
	float4 positionWorld: POSITION;
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
	float3 N = normalize(pin.positionWorld.xyz);
	float3 irradiance = float3(0.0, 0.0, 0.0);

	
	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));

	float sampleDelta = 0.025;
	float nrSamples = 0.0;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			// spherical to cartesian (in tangent space)
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += cubeMap.Sample(samAnisotropicWrap, sampleVec)  * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	return float4(irradiance, 1.0);

}

