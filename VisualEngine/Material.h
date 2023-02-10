#pragma once
#include "pch.h"
#include "Graphics.h"
#include <string>

struct Material 
{
	DirectX::XMFLOAT3 diffuse;
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
	std::string name;


	UINT MatCBIndex = -1;

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

