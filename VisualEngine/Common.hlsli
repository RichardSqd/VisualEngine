struct SurfaceProperties 
{
	float3 N;
	float3 V;
	float NdotV;
	float roughness; 

};

struct Light 
{
	float4 Color; //color of the lights
	float3 WorldDirection; // direction and spot lights
	float3 WorldPosition; //point and spot lights
	float SpotlightAngle; //spotlight half angle
	float RangeStart; //point and spot lights
	float RangeEnd; //point and spot lights
	float Intensity; 
	bool Enabled;
	uint Type; 
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

