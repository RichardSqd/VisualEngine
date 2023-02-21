
SamplerState samPointWrap	 : register(s0);
SamplerState samPointClamp  : register(s1);
SamplerState samLinearWrap	 : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAnisotropicWrap : register(s4);
SamplerState samAnisotropicClamp : register(s5);


Texture2D diffuseMap : register(t1);
//Texture2D metallicRoughnessMap : register();


cbuffer cbPerObject : register(b0)
{
	float4x4 WorldMatrix;
	float4x4 WorldITMatrix;
};

cbuffer cbGlobal : register(b1)
{
	float4x4 ViewProjMatrix;
	float3 CameraPos;
	float NearZ;
	float FarZ;
}

cbuffer cbMaterial : register(b2)
{
	float4 diffuseFactor;
	float metallicFactor;
	float roughnessFactor;

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
	vout.positionWorld = mul(float4(vin.pos,1), WorldMatrix);
	vout.position = mul(vout.positionWorld, ViewProjMatrix);
	//normal vector transformation 
	vout.normal = mul(WorldITMatrix, float4(vin.normal,1)*2-1).xyz;
	vout.tangent = mul(WorldITMatrix, float4(vin.tangent, 1) *2-1);
	vout.uv = vin.tex;
	//vout.color = float4(1.0f, 0.0f, 0.0f, 0.5f);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = diffuseFactor * diffuseMap.Sample(samLinearClamp, pin.uv);
	//float2 metallicRoughnessFactor = float2(metallicFactor, roughnessFactor);
	//float2 metallicRoughness = metallicRoughnessFactor * 

	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	
	
	return  diffuseAlbedo;
}