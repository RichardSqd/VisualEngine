#include "pch.h"
#include "Cubemap.h"



namespace Cubemap {
	ComPtr<ID3D12Resource> vertexPosBufferGPU;
	ComPtr<ID3D12Resource> vertexPosUploader;
	ComPtr<ID3D12Resource> indexUploader;
	ComPtr<ID3D12Resource> indexBufferGPU;

	std::vector<byte> vertexPosBufferCPU;
	std::vector<byte> indexBufferCPU;

	long long indexBufferByteSize;
	long long vertexPosBufferByteSize;

	UINT indexCount;

	std::vector<std::unique_ptr<UploadBuffer>> passCB;
}