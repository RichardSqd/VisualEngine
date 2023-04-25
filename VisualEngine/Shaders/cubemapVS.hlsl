

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


struct VertexIn
{

	float3 pos : POSITION;
	//float3 normal : NORMAL;
	//float2 tex : TEXCOORD;
	//float3 tangent : TANGENT;
	//float4 color : COLOR;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
	//float4 tangent: TANGENT;
	float4 positionWorld: POSITION;
	//float3 normal : NORMAL;
	//float2 uv: TEXCOORD;
	
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.positionWorld =  float4(vin.pos, 1);
	vout.position = mul( vout.positionWorld, ViewProjMatrix);
	//normal vector transformation 
	//vout.normal = mul(   float4(vin.normal, 1) * 2 - 1, WorldITMatrix ).xyz;
	//vout.tangent = mul( float4(vin.tangent, 1) * 2 - 1, WorldITMatrix);
	//vout.uv = vin.tex;
	return vout;
}
