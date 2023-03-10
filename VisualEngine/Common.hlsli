struct SurfaceProperties 
{
	float3 N;
	float3 V;
	float3 diffuse;
	float NdotV;
	float roughness; 

};

struct Light 
{
	float4 Color; //color of the lights

	float3 WorldDirection; // direction and spot lights
	float SpotlightAngle; //spotlight half angle

	float3 WorldPosition; //point and spot lights
	float RangeStart; //point and spot lights

	float RangeEnd; //point and spot lights
	float Intensity; 
	uint Type; 
	bool Enabled;
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