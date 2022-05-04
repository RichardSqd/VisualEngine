
Texture2D shadowMap: register(t0);
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


