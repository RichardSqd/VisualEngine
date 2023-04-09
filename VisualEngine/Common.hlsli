#define VE_GPU 1 
#include "ShaderLightingData.h"

struct SurfaceProperties 
{
	float3 N;
	float3 V;
	float3 diffuse;
	float NdotV;
	float roughness; 

};




float3 ComputeDirectionalLight() {

}

float3 ComputeSpotLight() {

}

float3 ComputePointLight() {

}


float4 ComputeLighting() {
	ComputeDirectionalLight();
}

//I(d) = I0 / (d^2)
//Linear Attenuation 
float computeAttenuation(float d, float falloffStart, float falloffEnd) {
	return saturate((falloffEnd - d)) / (falloffEnd - falloffStart);
}

//Schlick approximation (simplified), Real Time Rendering P321
//F0 = ((n-1)/(n+1))^2
float3 SchlickFresnel(float3 r0, float3 n, float3 L) {
	//float cosIncidentAngle;
}

float4 ComputePhongLighting(SurfaceProperties surface, SceneLighting lights, float3 cameraPos, float3 positionWorld ) {

	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	float3 lightDir = normalize(lights.directionalLights[0].lightDirection - positionWorld);
	float3 norm = surface.N;
	float4 diffuseLgt = max(dot(norm, lightDir), 0.0);

	float specularStrength = 0.5;
	float3 viewDir = normalize(cameraPos - positionWorld);
	float3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	float4 specLgt = float4(specularStrength * spec * lights.directionalLights[0].color, 0.0);
	return (diffuseLgt + ambientLight + specLgt);
}
