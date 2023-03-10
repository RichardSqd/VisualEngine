
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

	bool HasDiffuseTexture; 
	bool HasMetallicRoughnessTexture;
	bool HasNormalTexture; 
	bool HasOcclusionTexture; 
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
	vout.positionWorld = mul(WorldMatrix, float4(vin.pos, 1));
	vout.position = mul( ViewProjMatrix, vout.positionWorld );
	//normal vector transformation 
	vout.normal = mul(  WorldITMatrix, float4(vin.normal, 1) * 2 - 1 ).xyz;
	vout.tangent = mul( WorldITMatrix, float4(vin.tangent, 1) * 2 - 1);
	vout.uv = vin.tex;
	//vout.color = float4(1.0f, 0.0f, 0.0f, 0.5f);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo;
	float2 metallicRoughnessFactor;

	if (HasDiffuseTexture) {
		diffuseAlbedo = DiffuseFactor * diffuseMap.Sample(samPointWrap, pin.uv);
	}
	else {
		diffuseAlbedo = DiffuseFactor;
	}

	if (HasMetallicRoughnessTexture) {
		metallicRoughnessFactor = float2(MetallicFactor, RoughnessFactor);
	}
	else {
		metallicRoughnessFactor = float2(0.25, 0.25);
	}
	
	float2 metallicRoughness = metallicRoughnessFactor * metallicRoughnessMap.Sample(samLinearClamp, pin.uv).bg;

	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	
	SurfaceProperties surface; 
	surface.N = pin.normal;
	surface.V = normalize(CameraPos - pin.positionWorld.xyz);
	surface.NdotV = saturate(dot(surface.N, surface.V));
	surface.diffuse = diffuseAlbedo.rgb * (1 - dielectricSpecular) * (1 - metallicRoughness.x) * 1;

	return float4(surface.diffuse, diffuseAlbedo.a);
}

float3 ComputeNormal(VertexOut pin) {
	float3 normal = normalize(pin.normal);
	float3 tangent = normalize(pin.tangent.xyz);
	float3 bitangent = normalize(cross(normal, tangent)) * pin.tangent.w;
	float3x3 tangentMatrix = float3x3(tangent, bitangent, normal);

	//sample normal map 
	normal = (normalMap.Sample(samLinearClamp, pin.uv) * 2 - 1).xyz;

	//scaling
	

	return mul(normal, tangentMatrix);

}