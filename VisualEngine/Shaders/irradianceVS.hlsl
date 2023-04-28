

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

};

struct VertexOut
{
	float4 position  : SV_POSITION;
	float4 positionWorld: POSITION;

	
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.positionWorld =  float4(vin.pos, 1);
	vout.position = mul( vout.positionWorld, ViewProjMatrix);

	return vout;
}
