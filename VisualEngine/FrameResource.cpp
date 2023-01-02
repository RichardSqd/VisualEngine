#include "pch.h"
#include "FrameResource.h"
#include "Graphics.h"
#include "EngineCore.h"


FrameResource::FrameResource() {

}

FrameResource::FrameResource(UINT32 passCount, UINT32 objectCount) {
	comandContext = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	passCB = std::make_unique<UploadBuffer>(sizeof(PassConstants), passCount, true);
	objCB = std::make_unique<UploadBuffer>(sizeof(ObjectConstants),objectCount, true);

}

UINT FrameResourceManager::curFrameIndex = 0;

FrameResourceManager::FrameResourceManager()
{
	mFrameResources.clear();
}

FrameResourceManager::~FrameResourceManager()
{
}

void FrameResourceManager::CreateFrameResources(UINT numberOfFrameResources)
{
	for (UINT i = 0; i < numberOfFrameResources; i++) {
		mFrameResources.push_back(std::make_unique<FrameResource>(1, EngineCore::eModel.numMeshes));
	}
}

FrameResource* FrameResourceManager::GetCurrentFrameResource()
{
	return mFrameResources[curFrameIndex].get();
}
