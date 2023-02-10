#include "pch.h"
#include "FrameResource.h"
#include "Graphics.h"
#include "EngineCore.h"


FrameResource::FrameResource() {

}

FrameResource::FrameResource(UINT passCount, UINT objectCount, UINT materialCount) {
	comandContext = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	passCB = std::make_unique<UploadBuffer>(sizeof(PassConstants), passCount, true);
	objCB = std::make_unique<UploadBuffer>(sizeof(ObjectConstants),objectCount, true);
	matCB = std::make_unique<UploadBuffer>(sizeof(MaterialConstants), materialCount, true);
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
		std::wstring allocatorName = L"Allocator for frame " + std::to_wstring(i);
		mFrameResources.back()->comandContext->getCommandAllocator()->SetName(allocatorName.c_str());
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
