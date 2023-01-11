#include "pch.h"
#include "UploadBuffer.h"
#include "Graphics.h"





UploadBuffer::UploadBuffer(size_t elementSize,size_t numElement, bool isConstBuffer)
	:mNumElement(numElement), mIsConstBuffer(isConstBuffer) {

	mElementByteSize = elementSize;

	if (isConstBuffer) {
		mElementByteSize = (mElementByteSize + 255) & ~255;
	}
	mBufferByteSize = mElementByteSize * mNumElement;

	BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mBufferByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mUploadBuffer)));

	
	Map();

}


void UploadBuffer::Map() {

	BREAKIFFAILED(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
}

void UploadBuffer::CopyData(int elementIndex, const void* data) {
	memcpy(&mMappedData[elementIndex * mElementByteSize], data, mElementByteSize);
}