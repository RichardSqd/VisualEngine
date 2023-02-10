
SamplerState gsamPointWrap	 : register(s0);
SamplerState gsamPointClamp  : register(s1);
SamplerState gsamLinearWrap	 : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);


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
	float3 roughness;

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
	float4 color : COLOR;
	float3 positionWorld: POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD0;
	
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.positionWorld = mul(vin.pos, WorldMatrix);
	vout.position = mul(float4(vout.positionWorld,1), ViewProjMatrix);
	//normal vector transformation 
	vout.normal = mul(WorldITMatrix, vin.normal*2-1);
	vout.tangent = mul(WorldITMatrix, vin.tangent*2-1);
	
	vout.color = float4(1.0f, 0.0f, 0.0f, 0.5f);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	
	return pin.color;
}