#pragma once
#include "pch.h"
#include <string>

class Texture
{
public:
	std::string name;
	std::string uri;
	int width;
	int height;
	int component;
	int bits;
	int srvHeapIndex;
	int rtvHeapIndex;
	ComPtr<ID3D12Resource> textureResource;
	ComPtr<ID3D12Resource> textureUploader;

};
