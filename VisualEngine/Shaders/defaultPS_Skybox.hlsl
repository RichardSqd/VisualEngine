
#include "Common.hlsli"

SamplerState samPointWrap	 : register(s0);
SamplerState samPointClamp  : register(s1);
SamplerState samLinearWrap	 : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAnisotropicWrap : register(s4);
SamplerState samAnisotropicClamp : register(s5);


Texture2D diffuseMap : register(t1);
Texture2D metallicRoughnessMap : register(t2);
Texture2D normalMap : register(t3);
Texture2D occlusionMap: register(t4);
Texture2D emissiveMap: register(t5);



struct PixelIn
{
	float4 position  : SV_POSITION;
	float3 viewDir : TEXCOORD3;
	
};


float4 PS(PixelIn pin) : SV_Target
{
	return float4(radianceTexture.SampleLevel(defaultSampler, vsOutput.viewDir, TextureLevel), 1);

}

