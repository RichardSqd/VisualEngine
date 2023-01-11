#pragma once
#include "pch.h"
//#include "FrameResource.h"


class UploadBuffer {
public:

	UploadBuffer(size_t elementSize, size_t numElement, bool isConstBuffer);

	~UploadBuffer(){ mUploadBuffer = nullptr; };
	ID3D12Resource* GetResource() { return mUploadBuffer.Get(); };
	const ID3D12Resource* GetResource() const { return mUploadBuffer.Get(); };

	ID3D12Resource** GetAddressOf() { return mUploadBuffer.GetAddressOf(); };

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() { mUploadBuffer->GetGPUVirtualAddress(); }


	void Map(); 
	void UploadBuffer::CopyData(int elementIndex, const void* data);


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	bool mIsConstBuffer;
	size_t mBufferByteSize;
	size_t mElementByteSize;
	size_t mNumElement;
	BYTE* mMappedData;
};