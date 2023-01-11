#pragma once
#include "pch.h"
#include "CommandListManager.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;

class ObjectConstants {
public:
	ObjectConstants() {
		XMStoreFloat4x4(&World, Math::IdentityMatrix());
		XMStoreFloat4x4(&WorldIT, Math::IdentityMatrix());
	}
	DirectX::XMFLOAT4X4 World; 
	DirectX::XMFLOAT4X4 WorldIT;
};

class PassConstants {
public:
	PassConstants() {
		XMStoreFloat4x4(&ViewProjMatrix, Math::IdentityMatrix());
		//XMStoreFloat4x4(&CameraPos, Math::IdentityMatrix());
		NearZ = 0.0;
		FarZ = 0.0;

	}

	DirectX::XMFLOAT4X4 ViewProjMatrix;
	//DirectX::XMFLOAT4X4 CameraPos;
	float NearZ;
	float FarZ;

};




class FrameResource {
public:
	FrameResource();
	FrameResource(UINT32 passCount, UINT32 objectCount);
	//FrameResource& operator=(const FrameResource rhs);


	std::shared_ptr<CommandContext> comandContext;


	std::unique_ptr<UploadBuffer> objCB = nullptr;
	std::unique_ptr<UploadBuffer> passCB = nullptr;

	UINT64 fence = 0;

};

class FrameResourceManager {
public:
	FrameResourceManager();
	~FrameResourceManager();
	void CreateFrameResources(UINT numberOfFrameResources);
	FrameResource* GetCurrentFrameResource();
	void nextFrameResource();
	UINT GetCurrentIndex();
	FrameResource* GetFrameResourceByIndex(UINT index);

private:
	static UINT curFrameIndex ;
	UINT numFrameResources;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;




};