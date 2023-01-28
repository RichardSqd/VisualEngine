
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
	float4x4 ViewProjMatrix;
	float3 CameraPos;
	float NearZ;
	float FarZ;
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
	float3 normal : NORMAL;
	float4 color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.position = mul(float4(vin.pos, 1.0), WorldMatrix);
	vout.position = mul(vout.position, ViewProjMatrix);
	vout.normal = vin.normal;
	//float4 color2 = float4(1.0f, 0.0f, 0.0f, 1.0f);
	//float4 color1 = float4(0.0f, 0.0f, 1.0f, 1.0f);
	//float d = distance(float4(CameraPos, 0), vin.world);
	//float4 a = lerp(color1,color2, d);
	vout.color = float4(1.0f, 0.0f, 0.0f, 0.5f);
	//vout.normal = float3(1.0f, 1.0f, 1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	
	return pin.color;
}