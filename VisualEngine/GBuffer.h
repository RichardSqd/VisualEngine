#pragma once
#include "pch.h"
#include "Texture.h"

class GBuffer {
public:
	static DXGI_FORMAT depthFormat;
	static DXGI_FORMAT positionFormat;
	static DXGI_FORMAT diffuseFormat;
	static DXGI_FORMAT specularFormat;
	static DXGI_FORMAT normalFormat;
	static DXGI_FORMAT emissiveFormat;

	Texture gbDepth;
	Texture gbPosition;
	Texture gbDiffuse;
	Texture gbSpecular;
	Texture gbNormal;
	Texture gbEmissive;
	 
	void Init(UINT width, UINT height);
	void BuildSRVs(int heapIndex);
	void BuildRTVs(int heapIndex);
	void Clear();
};