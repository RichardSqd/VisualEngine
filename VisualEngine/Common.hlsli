#define VE_GPU 1 
#include "ShaderLightingData.h"

static const float PI = 3.14159265;
static const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);

struct SurfaceProperties 
{
	float3 N;
	float NdotV;
	float3 V;


};

struct MaterialParameters
{
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
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
float3 SchlickFresnel(float3 cosTheta, float3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness) {
	float a2 = roughness * roughness * roughness * roughness;
	float NdotH = max(dot(N, H), 0.0);

	float num = a2;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	
	return num / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {

	return  GeometrySchlickGGX(max(dot(N, L), 0.0), roughness) *
			GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
}


float4 ComputePhongLighting(SurfaceProperties surface, SceneLighting lights, float3 cameraPos, float3 positionWorld ) {

	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	float3 lightDir = normalize(lights.directionalLights[0].lightDirection - positionWorld) * lights.directionalLights[0].strength;
	float3 norm = surface.N;
	float4 diffuseLgt = max(dot(norm, lightDir), 0.0) ;

	float specularStrength = 0.5;
	float3 viewDir = normalize(cameraPos - positionWorld);
	float3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	float4 specLgt = float4(specularStrength * spec * lights.directionalLights[0].color * lights.directionalLights[0].strength, 0.0);
	return (diffuseLgt + ambientLight + specLgt);
}

float4 ComputeBlinnPhongLighting(SurfaceProperties surface, SceneLighting lights, float3 cameraPos, float3 positionWorld) {
	float4 ambientLight = float4(0.25f, 0.25f, 0.35f, 1.0f);
	float3 lightDir = normalize(lights.directionalLights[0].lightDirection - positionWorld) * lights.directionalLights[0].strength;
	float3 norm = surface.N;
	float4 diffuseLgt = max(dot(norm, lightDir), 0.0);

	float specularStrength = 0.5;
	float3 viewDir = normalize(cameraPos - positionWorld);
	float3 halfwayDir = normalize(lightDir + viewDir);
	float3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
	float4 specLgt = float4(specularStrength * spec * lights.directionalLights[0].color * lights.directionalLights[0].strength, 0.0);
	return (diffuseLgt + ambientLight + specLgt);
}


float4 ComputePBRLighting(SurfaceProperties surface, SceneLighting lights, MaterialParameters materialParams, float3 cameraPos, float3 worldPos, float3 irradiance) {
	
	//materialParams.metallic = lights.directionalLights[0].strength.x;
	//materialParams.roughness = lights.directionalLights[0].strength.y;
	float ao = 1.0; 
	float3 f0 = float3(0.04, 0.04, 0.04);
	f0 = lerp(f0, materialParams.albedo, materialParams.metallic);
	
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < lights.numDirectionalLights; i++) {
		
		float3 lightPos = lights.directionalLights[i].lightDirection;
		float3 lightColor = lights.directionalLights[i].color;
		float3 V = normalize(cameraPos - lightPos);
		float3 L = normalize(lightPos - worldPos);
		float3 H = normalize(V + L);
		float distance = length(lightPos - worldPos);
		float attenuation = 1.0 / (distance * distance);
		float3 radiance = lightColor * attenuation;

		//brdf 
		float NDF = DistributionGGX(surface.N, H, materialParams.roughness);
		float G = GeometrySmith(surface.N, V, L, materialParams.roughness);
		float3 F = SchlickFresnel(max(dot(H, V),0.0), f0);

		float3 k5 = F;
		float3 kD = float3(1.0, 1.0, 1.0) - k5;
		kD *= 1.0 - materialParams.metallic;

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(surface.N, V), 0.0) * max(dot(surface.N, L), 0.0);
		float3 specular = numerator / denominator;

		float NdotL = max(dot(surface.N, L), 0.0);
		Lo += (kD * materialParams.albedo / PI + specular) * radiance * NdotL;
		
	}

	//float3 ks = SchlickFresnel(max(dot(surface.N, normalize(cameraPos - worldPos)), 0.0), f0);
	//float3 kd = 1.0 - ks; 
	//kd *= 1.0 - materialParams.metallic;
	//float3 diffuse = irradiance * materialParams.albedo;
	//float3 ambient =  kd * diffuse * ao;

	float3 ambient = float3(0.03, 0.03, 0.03) * materialParams.albedo;// *materialParams.ao;
	float3 color = ambient + Lo;

	//gamma correct
	//color = color / (color + float3(1.0, 1.0, 1.0));
	//color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
	
	return float4(color, 1.0f);
}