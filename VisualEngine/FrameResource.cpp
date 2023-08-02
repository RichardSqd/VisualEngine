#include "pch.h"
#include "FrameResource.h"
#include "Graphics.h"
#include "EngineCore.h"
#include "Cubemap.h"

LightConstants::LightConstants() {
	lights.numDirectionalLights = 0;
	lights.numSpotLights = 0;
	lights.numPointLights = 0;
}


FrameResource::FrameResource() {

}

FrameResource::FrameResource(UINT passCount, UINT objectCount, UINT materialCount) {
	
	//initalize command contexts for each threads 
	shadowComandContexts.reserve(Config::NUMCONTEXTS);
	for (int i = 0; i < Config::NUMCONTEXTS; i++) {
		shadowComandContexts.push_back( Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT));
		std::wstring allocatorName = L"Shadow allocator for thread # " + std::to_wstring(i);
		shadowComandContexts[i]->getCommandAllocator()->SetName(allocatorName.c_str());
	}

	sceneComandContexts.reserve(Config::NUMCONTEXTS);
	for (int i = 0; i < Config::NUMCONTEXTS; i++) {
		sceneComandContexts.push_back(Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT));
		std::wstring allocatorName = L"Scene allocator for thread # " + std::to_wstring(i);
		sceneComandContexts[i]->getCommandAllocator()->SetName(allocatorName.c_str());
	}

	mainContextPre = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mainContextMid = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mainContextPost = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	passCB = std::make_unique<UploadBuffer>(sizeof(PassConstants), passCount, true);
	objCB = std::make_unique<UploadBuffer>(sizeof(ObjectConstants),objectCount, true);
	matCB = std::make_unique<UploadBuffer>(sizeof(MaterialConstants), materialCount, true);
	lightCB = std::make_unique<UploadBuffer>(sizeof(LightConstants), 1, true);
	shadowCB = std::make_unique<UploadBuffer>(sizeof(ShadowConstants), 1, true);

	CreateShadowMapAsset();

	//initalize gbuffer
	gbuffer.Init(Graphics::gWidth, Graphics::gHeight);

}

void FrameResource::CreateShadowMapAsset() {
	CD3DX12_RESOURCE_DESC shadowMapDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT>(Graphics::gWidth),
		static_cast<UINT>(Graphics::gHeight),
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	//create shadow map resource for the frame
	BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&shadowMapDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&clearValue,
		IID_PPV_ARGS(&shadowMap)));
	
}

UINT FrameResourceManager::curFrameIndex = Graphics::gNumFrameResources - 1;

FrameResourceManager::FrameResourceManager()
{
	numFrameResources = 0;
	mFrameResources.clear();
}

FrameResourceManager::~FrameResourceManager()
{
}

UINT FrameResourceManager::GetCurrentIndex() {
	return curFrameIndex;
}

void FrameResourceManager::CreateFrameResources(UINT numberOfFrameResources)
{
	numFrameResources = numberOfFrameResources;
	for (UINT i = 0; i < numberOfFrameResources; i++) {
		mFrameResources.push_back(std::make_unique<FrameResource>(1, EngineCore::eModel.numNodes, EngineCore::eModel.materials.size()));
		
	}

	// create cubemap rendering constant buffer
	for (int i = 0; i < 6; i++) {
		Cubemap::passCB.push_back(std::make_unique<UploadBuffer>(sizeof(PassConstants), 1, true));
	}



}

FrameResource* FrameResourceManager::GetCurrentFrameResource()
{
	return mFrameResources[curFrameIndex].get();
}

void FrameResourceManager::NextFrameResource()
{
	curFrameIndex = (curFrameIndex + 1) % numFrameResources;
}

FrameResource* FrameResourceManager::GetFrameResourceByIndex(UINT index)
{
	return mFrameResources[index].get();
}
