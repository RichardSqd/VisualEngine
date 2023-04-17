
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
		metallicRoughnessFactor = float2(0.25, 0.25);
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

	//

	//diffuseAlbedo = // diffuseAlbedo + roughnessmetallic; + emissiveAlbedo + roughnessmetallic + normalAlbedo + aoAlbedo;
	//if (HasEmissiveTexture == 1) {
		//emissiveAlbedo = 
	//}
	



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
	

	//phong lighting 
	float4 pbrLighting = ComputePBRLighting(surface, lights, materialParams, CameraPos, pin.positionWorld.xyz);


	
	return float4(pbrLighting.rgb+ emissive.rgb, diffuseAlbedo.a);
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