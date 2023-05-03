
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

TextureCube<float3> specularCubeMap : register(t6);
TextureCube<float3> irradianceCubeMap : register(t7);


float3 diffuseIBL(SurfaceProperties surface, MaterialParameters materialParams);
float3 specularIBL(SurfaceProperties surface, MaterialParameters materialParams);

cbuffer cbGlobal : register(b1)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 CameraPos;
	float NearZ;
	int shaderSelector;
	float FarZ;
}

cbuffer cbMaterial : register(b2)
{
	float4 DiffuseFactor;
	float MetallicFactor;
	float RoughnessFactor;
	int HasDiffuseTexture; 
	int HasMetallicRoughnessTexture;

	int HasNormalTexture;
	int HasOcclusionTexture;
	int HasEmissiveTexture;
	int dummy1;

	
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
	float4 diffuseAlbedo;
	float2 metallicRoughnessFactor;
	float4 roughnessmetallic;
	float4 emissive;
	//float4 normalAlbedo;
	float ao;
	if (HasDiffuseTexture==1) {
		//float4 temp = DiffuseFactor * metallicRoughnessMap.Sample(samPointWrap, pin.uv);
		//diffuseAlbedo += temp;
		diffuseAlbedo = DiffuseFactor * diffuseMap.Sample(samPointWrap, pin.uv);
	}
	else {
		diffuseAlbedo = DiffuseFactor;
	}

	if (HasMetallicRoughnessTexture == 1) {
		roughnessmetallic =  metallicRoughnessMap.Sample(samPointWrap, pin.uv);
	}
	else {
		roughnessmetallic = float4(0.25, 0.25, 0.25, 0.25);
	}

	
	if (HasEmissiveTexture == 1) {
		emissive =  emissiveMap.Sample(samPointWrap, pin.uv);
	}
	else {
		emissive = float4(0, 0, 0, 0);
	}
	/*
	if (HasNormalTexture) {
		normalAlbedo = float4(0.1, 0.1, 0.1, 0.1) * normalMap.Sample(samPointWrap, pin.uv);
	}
	*/
	if (HasOcclusionTexture) {
		ao =  occlusionMap.Sample(samPointWrap, pin.uv).r;
	}
	else {
		ao = 1.0;
	}

	
	float2 metallicRoughness = metallicRoughnessFactor * metallicRoughnessMap.Sample(samLinearClamp, pin.uv).bg;

	
	
	SurfaceProperties surface; 
	surface.N = normalize(pin.normal);
	surface.V = normalize(CameraPos - pin.positionWorld.xyz);
	surface.NdotV = saturate(dot(surface.N, surface.V));
	
	MaterialParameters materialParams;
	materialParams.albedo = diffuseAlbedo.rgb;
	materialParams.metallic = diffuseAlbedo.b;
	materialParams.roughness = roughnessmetallic.g;
	materialParams.ao = 0;
	
	//irradiance 
	float3 irradiance = irradianceCubeMap.Sample(samAnisotropicWrap, surface.N);

	//phong lighting 
	float4 pbrLighting = ComputePBRLighting(surface, lights, materialParams, CameraPos, pin.positionWorld.xyz, irradiance);


	float3 dIBL = diffuseIBL(surface, materialParams);
	float3 sIBL = specularIBL(surface, materialParams);
	float3 color = dIBL + sIBL + emissive.rgb;//+ pbrLighting.rgb;
	return float4(color, 1.0);
}

float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
	float pow5 = (1.0 - cosine) * (1.0 - cosine) * (1.0 - cosine) * (1.0 - cosine) * (1.0 - cosine);
	return lerp(F0, F90, pow5);
}

float3 diffuseIBL(SurfaceProperties surface, MaterialParameters materialParams) {
	float3 cdiff = materialParams.albedo * (1 - materialParams.metallic) * (1 - float3(0.04, 0.04, 0.04));
	float LdotH = saturate(dot(surface.N, normalize(surface.N + surface.V)));
	float fd90 = 0.5 + 2.0 * materialParams.roughness * LdotH * LdotH;
	float3 DiffuseBurley = cdiff * Fresnel_Shlick(1, fd90, surface.NdotV);
	return DiffuseBurley * irradianceCubeMap.Sample(samAnisotropicWrap, surface.N) * 2.5;
}

float3 specularIBL(SurfaceProperties surface, MaterialParameters materialParams) {
	float lod = materialParams.roughness * 10 + 0;
	float3 c_spec = lerp(float3(0.04, 0.04, 0.04), materialParams.albedo, materialParams.metallic)* 1.0;
	float3 specular = Fresnel_Shlick(c_spec, 1, surface.NdotV);
	return specular * specularCubeMap.Sample(samAnisotropicWrap, reflect(-surface.V, surface.N));

}


float3 ComputeNormal(PixelIn pin) {
	float3 normal = normalize(pin.normal);
	float3 tangent = normalize(pin.tangent.xyz);
	float3 bitangent = normalize(cross(normal, tangent)) * pin.tangent.w;
	float3x3 tangentMatrix = float3x3(tangent, bitangent, normal);

	//sample normal map 
	normal = (normalMap.Sample(samLinearClamp, pin.uv) * 2 - 1).xyz;

	//scaling
	

	return mul( tangentMatrix, normal);

}