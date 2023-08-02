#include "pch.h"
#include "GBuffer.h"
#include "EngineCore.h"

DXGI_FORMAT GBuffer::depthFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
DXGI_FORMAT GBuffer::positionFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
DXGI_FORMAT GBuffer::diffuseFormat;
DXGI_FORMAT GBuffer::specularFormat;
DXGI_FORMAT GBuffer::normalFormat;
DXGI_FORMAT GBuffer::emissiveFormat;

void GBuffer::Init(UINT width, UINT height) {
	auto& model = EngineCore::eModel;
	auto& device = Graphics::gDevice;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.MipLevels = 1;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.Flags =  D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	texDesc.DepthOrArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_RESOURCE_DESC positionDesc = texDesc;
	positionDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	D3D12_RESOURCE_DESC diffuseDesc = texDesc;
	diffuseDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;

	D3D12_RESOURCE_DESC specularDesc = texDesc;
	specularDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_RESOURCE_DESC normalDesc = texDesc;
	normalDesc.Format = DXGI_FORMAT_R16G16B16A16_SNORM;

	D3D12_RESOURCE_DESC emissiveDesc = texDesc;
	emissiveDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	
	D3D12_CLEAR_VALUE clearValue = {};
	memcpy(clearValue.Color, DirectX::Colors::Black, sizeof(float) * 4);

	clearValue.Format = positionDesc.Format;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&positionDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(this->gbPosition.textureResource.GetAddressOf()));
	this->gbPosition.name = "GBuffer_Position";

	clearValue.Format = diffuseDesc.Format;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&diffuseDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(this->gbDiffuse.textureResource.GetAddressOf()));
	this->gbDiffuse.name = "GBuffer_Diffuse";

	clearValue.Format = specularDesc.Format;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&specularDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(this->gbSpecular.textureResource.GetAddressOf()));
	this->gbSpecular.name = "GBuffer_Specular";

	clearValue.Format = normalDesc.Format;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&normalDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(this->gbNormal.textureResource.GetAddressOf()));
	this->gbNormal.name = "GBuffer_Normal";

	clearValue.Format = emissiveDesc.Format;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&emissiveDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(this->gbEmissive.textureResource.GetAddressOf()));
	this->gbEmissive.name = "GBuffer_Emissive";

}

void GBuffer::BuildSRVs(int heapIndex) {
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//position 
	auto& posTexResource = this->gbPosition.textureResource;
	srvDesc.Format = posTexResource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = posTexResource->GetDesc().MipLevels;
	CD3DX12_CPU_DESCRIPTOR_HANDLE posHandle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	posHandle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
	Graphics::gDevice->CreateShaderResourceView(posTexResource.Get(), &srvDesc, posHandle);
	this->gbPosition.srvHeapIndex = heapIndex;
	heapIndex += 1;

	//diffuse 
	auto& diffuseTexResource = this->gbDiffuse.textureResource;
	srvDesc.Format = diffuseTexResource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = diffuseTexResource->GetDesc().MipLevels;
	CD3DX12_CPU_DESCRIPTOR_HANDLE diffuseHandle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	diffuseHandle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
	Graphics::gDevice->CreateShaderResourceView(diffuseTexResource.Get(), &srvDesc, diffuseHandle);
	this->gbDiffuse.srvHeapIndex = heapIndex;
	heapIndex += 1;
	
	//specular 
	auto& specularTexResource = this->gbSpecular.textureResource;
	srvDesc.Format = specularTexResource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = specularTexResource->GetDesc().MipLevels;
	CD3DX12_CPU_DESCRIPTOR_HANDLE specularHandle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	specularHandle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
	Graphics::gDevice->CreateShaderResourceView(specularTexResource.Get(), &srvDesc, specularHandle);
	this->gbSpecular.srvHeapIndex = heapIndex;
	heapIndex += 1;

	//normal 
	auto& normalTexResource = this->gbNormal.textureResource;
	srvDesc.Format = normalTexResource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = normalTexResource->GetDesc().MipLevels;
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalHandle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	normalHandle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
	Graphics::gDevice->CreateShaderResourceView(normalTexResource.Get(), &srvDesc, normalHandle);
	this->gbNormal.srvHeapIndex = heapIndex;
	heapIndex += 1;

	//emissive 
	auto& emissiveTexResource = this->gbEmissive.textureResource;
	srvDesc.Format = emissiveTexResource->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = emissiveTexResource->GetDesc().MipLevels;
	CD3DX12_CPU_DESCRIPTOR_HANDLE emissiveHandle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	emissiveHandle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
	Graphics::gDevice->CreateShaderResourceView(emissiveTexResource.Get(), &srvDesc, emissiveHandle);
	this->gbEmissive.srvHeapIndex = heapIndex;
}

void GBuffer::BuildRTVs(int heapIndex) {
	

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//position 
	auto& posTexResource = this->gbPosition.textureResource;
	rtvDesc.Format = posTexResource->GetDesc().Format;
	CD3DX12_CPU_DESCRIPTOR_HANDLE posRtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
	posRtvHandle.Offset(heapIndex, Graphics::gRTVDescriptorSize);
	Graphics::gDevice->CreateRenderTargetView(posTexResource.Get(), &rtvDesc, posRtvHandle);
	this->gbPosition.rtvHeapIndex = heapIndex;
	heapIndex += 1;

	//diffuse 
	auto& diffuseTexResource = this->gbDiffuse.textureResource;
	rtvDesc.Format = diffuseTexResource->GetDesc().Format;
	CD3DX12_CPU_DESCRIPTOR_HANDLE diffuseRtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
	diffuseRtvHandle.Offset(heapIndex, Graphics::gRTVDescriptorSize);
	Graphics::gDevice->CreateRenderTargetView(diffuseTexResource.Get(), &rtvDesc, diffuseRtvHandle);
	this->gbDiffuse.rtvHeapIndex = heapIndex;
	heapIndex += 1;


	//specular 
	auto& specularTexResource = this->gbSpecular.textureResource;
	rtvDesc.Format = specularTexResource->GetDesc().Format;
	CD3DX12_CPU_DESCRIPTOR_HANDLE specularRtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
	specularRtvHandle.Offset(heapIndex, Graphics::gRTVDescriptorSize);
	Graphics::gDevice->CreateRenderTargetView(specularTexResource.Get(), &rtvDesc, specularRtvHandle);
	this->gbSpecular.rtvHeapIndex = heapIndex;
	heapIndex += 1;

	//normal 
	auto& normalTexResource = this->gbNormal.textureResource;
	rtvDesc.Format = normalTexResource->GetDesc().Format;
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalRtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
	normalRtvHandle.Offset(heapIndex, Graphics::gRTVDescriptorSize);
	Graphics::gDevice->CreateRenderTargetView(normalTexResource.Get(), &rtvDesc, normalRtvHandle);
	this->gbNormal.rtvHeapIndex = heapIndex;
	heapIndex += 1;

	//emissive 
	auto& emissiveTexResource = this->gbEmissive.textureResource;
	rtvDesc.Format = emissiveTexResource->GetDesc().Format;
	CD3DX12_CPU_DESCRIPTOR_HANDLE emissiveRtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
	emissiveRtvHandle.Offset(heapIndex, Graphics::gRTVDescriptorSize);
	Graphics::gDevice->CreateRenderTargetView(emissiveTexResource.Get(), &rtvDesc, emissiveRtvHandle);
	this->gbEmissive.rtvHeapIndex = heapIndex;
	heapIndex += 1;
}

void GBuffer::Clear() {

}