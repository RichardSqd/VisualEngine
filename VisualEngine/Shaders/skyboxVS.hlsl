

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
	uint vertID : SV_VertexID;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
	float3 viewDir: TEXCOORD3;
};

VertexOut VS(VertexIn vin)
{

	float4x4 proj = float4x4(	0.55228, 0.0, 0.0, 0.0,
								0.0, 0.414213568, 0.0, 0.0,
								0.0, 0.0, 0.0, -9.9990,
								0.0, 0.0, -1, 9.99900);
	
	VertexOut vout;
	float2 screenUV = float2(uint2(vin.vertID, vin.vertID << 1) & 2);
	float4 projectedPos = float4(lerp(float2(-1, 1), float2(1, -1), screenUV), 0, 1);
	float4 posViewSpace = mul(projectedPos, proj);

	vout.position = projectedPos;
	float4 p = float4(posViewSpace.xyz , 0);
	vout.viewDir = mul(p, ViewMatrix).xyz;
	

	return vout;
}
