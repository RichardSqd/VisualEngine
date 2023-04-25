#pragma once
#include "pch.h"

namespace Cubemap {

	extern ComPtr<ID3D12Resource> vertexPosBufferGPU;
	extern ComPtr<ID3D12Resource> vertexPosUploader;
	extern ComPtr<ID3D12Resource> indexUploader;
	extern ComPtr<ID3D12Resource> indexBufferGPU;

	extern std::vector<byte> vertexPosBufferCPU;
	extern std::vector<byte> indexBufferCPU;

	extern long long indexBufferByteSize;
	extern long long vertexPosBufferByteSize;

	extern UINT indexCount;

	extern std::vector<std::unique_ptr<UploadBuffer>> passCB;
};