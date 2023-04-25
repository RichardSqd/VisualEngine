



cbuffer cbGlobal : register(b1)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 CameraPos;
	float NearZ;
	float FarZ;
}

struct VertexOut
{
	float4 position  : SV_POSITION;
	float3 viewDir : TEXCOORD3;
	
};

VertexOut VS(uint vertID : SV_VertexID)
{
	VertexOut vout;

	float2 ScreenUV = float2(uint2(VertID, VertID << 1) & 2);
	float4 ProjectedPos = float4(lerp(float2(-1, 1), float2(1, -1), ScreenUV), 0, 1);
	float4 PosViewSpace = mul(ProjectedPos, ProjMatrix);

	vout.position = ProjectedPos;
	vout.viewDir = mul(ViewMatrix, PosViewSpace.xyz / PosViewSpace.w);

	return vout;
}
