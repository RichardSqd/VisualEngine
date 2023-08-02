#pragma once
#include "pch.h"
#include "Graphics.h"
#include <string>

struct Material 
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 specular;
	DirectX::XMFLOAT3 ambient;
	DirectX::XMFLOAT3 emissive;
	DirectX::XMFLOAT3 transparent;
	

	float roughness;
	float opacity;
	float specularParam;
	float metalness;

	std::string texDiffuseMap;
	std::string texSpecularMap;
	std::string texEmisiveMap;
	std::string texNormalMap;
	std::string texLightmapMap;
	std::string texReflectionMap;
	std::string texroughnessMetallicMap;
	std::string texOcclusionMap;
	std::string name;


	UINT matCBIndex = -1;
	UINT diffuseMapSrvHeapIndex = -1;
	UINT roughnessMetallicMapSrvHeaIndex = -1;
	UINT normalMapSrvHeapIndex = -1;
	UINT occlusionMapSrvHeapIndex = -1;
	UINT emissiveSrvHeapIndex = -1;

	int hasDiffuseTexture = 0;
	int hasMetallicRoughnessTexture = 0;
	int hasNormalTexture = 0;
	int hasOcclusionTexture = 0;
	int hasEmissiveTexture = 0;


	int numFrameDirty = Graphics::gNumFrameResources;


};


