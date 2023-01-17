
/*Texture2D shadowMap: register(t0);
Texture2D diffuseMap: register(t1);
Texture2D normalMap: register(t2);

SamplerState samplerWrap : register(s0);
SamplerState samplerClamp: register(s1);

cbuffer CB : register(b0) {
	float4x4 view;
}

struct VSIn {
	float4 world : POSITION;
	float3 normal : NORMAL;
	float3 tex : TEXCOORD;
	float3 tangent:TANGENT;
};

struct VSOut {
	float4 pos: SV_POSITION;
//	float4 world : POSITION;
//	float3 normal : NORMAL;
//	float3 tex : TEXCOORD;
//	float3 tangent:TANGENT;
};

VSOut VSMain(VSIn vin) {
	VSOut vsout;
	vsout.pos = mul(vin.world, view);
	return vsout;
}



float4 PSMain(VSOut vout) : SV_Target
{
	return vout.pos;
}
*/

cbuffer cbPerObject : register(b0)
{
	float4x4 WorldMatrix;
	float4x4 WorldITMatrix;
};

cbuffer cbGlobal : register(b1)
{
	float4x4 WorldViewProjMatrix;
	float3 CameraPos;
	float NearZ;
	float FarZ;
}

struct VertexIn
{

	float4 world : POSITION;
	float3 normal : NORMAL;
	float3 tex : TEXCOORD;
	float3 tangent : TANGENT;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
	//float3 normal : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.position = mul(vin.world, WorldViewProjMatrix);

	//vout.normal = float3(1.0f, 1.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return float4(1.0f,0.0f,0.0f,0.5f);
}