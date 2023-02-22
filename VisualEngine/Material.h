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
	std::string texEmmisiveMap;
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


	int numFrameDirty = Graphics::gNumFrameResources;


};

struct Texture
{
	std::string name;
	std::string uri;
	int width;
	int height;
	int component;
	int bits;
	std::unique_ptr<uint8_t[]> decodedData;
	ComPtr<ID3D12Resource> textureResource;
	ComPtr<ID3D12Resource> textureUploader;
	
};

