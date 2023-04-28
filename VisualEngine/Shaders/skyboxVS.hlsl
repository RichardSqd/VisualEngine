

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



	
	VertexOut vout;
	float2 screenUV = float2(uint2(vin.vertID, vin.vertID << 1) & 2);
	float4 projectedPos = float4(lerp(float2(-1, 1), float2(1, -1), screenUV), 0, 1);
	float4 posViewSpace = mul(projectedPos, ProjMatrix);

	vout.position = projectedPos;
	//float3x3 viewMat = float3x3(ViewMatrix[0].xyz, ViewMatrix[1].xyz, ViewMatrix[2].xyz);
	//float4 pvs = float4(posViewSpace.xyz / posViewSpace.w, 0);
	float4 p = float4(posViewSpace.xyz , 0);
	vout.viewDir = mul(p, ViewMatrix).xyz;
	

	/*
	VertexOut vout;
	float2 screenUV = float2(uint2(vin.vertID, vin.vertID << 1) & 2);
	float4 projectedPos = float4(lerp(float2(-1, 1), float2(1, -1), screenUV), 0, 1);
	float4 posViewSpace = mul(projectedPos, ProjMatrix);

	vout.position = projectedPos;
	//float3x3 viewMat = float3x3(ViewMatrix[0].xyz, ViewMatrix[1].xyz, ViewMatrix[2].xyz);
	//float4 pvs = float4(posViewSpace.xyz / posViewSpace.w, 0);
	float4 p = float4(posViewSpace.xyz, 0);
	vout.viewDir = mul(p, ViewMatrix).xyz;
	*/
	return vout;
}
