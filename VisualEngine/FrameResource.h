#pragma once
#include "pch.h"
#include "CommandListManager.h"
#include "UploadBuffer.h"
#include "ShaderLightingData.h"

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
		XMStoreFloat4x4(&ViewProjMatrix, DirectX::XMMatrixIdentity());
		//XMStoreFloat4x4(&CameraPos, Math::IdentityMatrix());
		XMStoreFloat4x4(&ViewMatrix, DirectX::XMMatrixIdentity());
		XMStoreFloat4x4(&ProjMatrix, DirectX::XMMatrixIdentity());
		CameraPos = {};
		NearZ = 0.0;
		FarZ = 0.0;

	}

	DirectX::XMFLOAT4X4 ViewProjMatrix;
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjMatrix;
	//DirectX::XMFLOAT4X4 CameraPos;
	DirectX::XMFLOAT3 CameraPos;
	float NearZ;
	float FarZ;

};

class MaterialConstants {

public:
	MaterialConstants(){
		diffuseFactor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		metallicFactor = 1.0f;
		roughnessFactor = 0.2f;

		hasDiffuseTexture = 0;
		hasMetallicRoughnessTexture = 0;
		hasNormalTexture = 0;
		hasOcclusionTexture = 0;
		hasEmissiveTexture = 0;
		dummy1 =0;

	}
	DirectX::XMFLOAT4 diffuseFactor;
	float metallicFactor;
	float roughnessFactor;
	int hasDiffuseTexture;
	int hasMetallicRoughnessTexture;

	int hasNormalTexture;
	int hasOcclusionTexture;
	int hasEmissiveTexture;
	int dummy1;
	
};


class LightConstant {
public:
	LightConstant();
	ShaderLightingData::SceneLighting lights;
};

class FrameResource {
public:
	FrameResource();
	FrameResource(UINT32 passCount, UINT32 objectCount, UINT32 materialCount);
	//FrameResource& operator=(const FrameResource rhs);


	std::shared_ptr<CommandContext> comandContext;


	std::unique_ptr<UploadBuffer> objCB = nullptr;
	std::unique_ptr<UploadBuffer> passCB = nullptr;
	std::unique_ptr<UploadBuffer> matCB = nullptr;
	std::unique_ptr<UploadBuffer> lightCB = nullptr;
	UINT64 fence = 0;

};

class FrameResourceManager {
public:
	FrameResourceManager();
	~FrameResourceManager();
	void CreateFrameResources(UINT numberOfFrameResources);
	FrameResource* GetCurrentFrameResource();
	void NextFrameResource();
	UINT GetCurrentIndex();
	FrameResource* GetFrameResourceByIndex(UINT index);

private:
	static UINT curFrameIndex ;
	UINT numFrameResources;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;




};