
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

static const float PI = 3.14159265;
static const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);

cbuffer cbPerObject : register(b0)
{
	float4x4 WorldMatrix;
	float4x4 WorldITMatrix;
};

cbuffer cbGlobal : register(b1)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 CameraPos;
	float NearZ;
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

struct VertexIn
{

	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
	float3 tangent : TANGENT;
	float4 color : COLOR;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
	float4 tangent: TANGENT;
	float4 positionWorld: POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD;
	
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.positionWorld = mul( float4(vin.pos, 1), WorldMatrix);
	vout.position = mul( vout.positionWorld, ViewProjMatrix);
	//normal vector transformation 
	vout.normal = mul(   float4(vin.normal, 1) * 2 - 1, WorldITMatrix ).xyz;
	vout.tangent = mul( float4(vin.tangent, 1) * 2 - 1, WorldITMatrix);
	vout.uv = vin.tex;
	//vout.color = float4(1.0f, 0.0f, 0.0f, 0.5f);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo;
	float2 metallicRoughnessFactor;
	float4 roughnessmetallic;
	float4 emissiveAlbedo;
	//float4 normalAlbedo;
	//float4 aoAlbedo;
	if (HasDiffuseTexture==1) {
		//float4 temp = DiffuseFactor * metallicRoughnessMap.Sample(samPointWrap, pin.uv);
		//diffuseAlbedo += temp;
		

		diffuseAlbedo = DiffuseFactor * diffuseMap.Sample(samPointWrap, pin.uv);
		
	}
	else {
		diffuseAlbedo = DiffuseFactor;
	}

	if (HasMetallicRoughnessTexture == 1) {
		metallicRoughnessFactor = float2(MetallicFactor, RoughnessFactor);
		roughnessmetallic = DiffuseFactor * metallicRoughnessMap.Sample(samPointWrap, pin.uv);
	}
	else {
		metallicRoughnessFactor = float2(0.25, 0.25);
	}

	/*
	if (HasEmissiveTexture == 1) {
		emissiveAlbedo = DiffuseFactor * emissiveMap.Sample(samPointWrap, pin.uv);

	}
	
	if (HasNormalTexture) {
		normalAlbedo = float4(0.1, 0.1, 0.1, 0.1) * normalMap.Sample(samPointWrap, pin.uv);
	}

	if (HasOcclusionTexture) {
		aoAlbedo = float4(0.1, 0.1, 0.1, 0.1) * occlusionMap.Sample(samPointWrap, pin.uv);
	}*/

	diffuseAlbedo = diffuseAlbedo;// + emissiveAlbedo + roughnessmetallic + normalAlbedo + aoAlbedo;
	//if (HasEmissiveTexture == 1) {
		//emissiveAlbedo = 
	//}
	
	float2 metallicRoughness = metallicRoughnessFactor * metallicRoughnessMap.Sample(samLinearClamp, pin.uv).bg;

	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	
	SurfaceProperties surface; 
	surface.N = pin.normal;
	surface.V = normalize(CameraPos - pin.positionWorld.xyz);
	surface.NdotV = saturate(dot(surface.N, surface.V));
	surface.diffuse = diffuseAlbedo.rgb * (1 - dielectricSpecular) * (1 - metallicRoughness.x) * 1;
	
	
	//if (lights.numDirectionalLights > 0) {
	//	diffuseAlbedo.rgb = float3(0.2, 0.3, 0.4);
	//}

	
	return float4(diffuseAlbedo.rgb, diffuseAlbedo.a);
}

float3 ComputeNormal(VertexOut pin) {
	float3 normal = normalize(pin.normal);
	float3 tangent = normalize(pin.tangent.xyz);
	float3 bitangent = normalize(cross(normal, tangent)) * pin.tangent.w;
	float3x3 tangentMatrix = float3x3(tangent, bitangent, normal);

	//sample normal map 
	normal = (normalMap.Sample(samLinearClamp, pin.uv) * 2 - 1).xyz;

	//scaling
	

	return mul( tangentMatrix, normal);

}