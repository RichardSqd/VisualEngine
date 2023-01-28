#pragma once
#include "Model.h"
#include "tiny_gltf.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

namespace Scene {
	extern int translate(tinygltf::Model& tinyModel, Scene::Model& model);
	extern int LoadScene(std::wstring filename, Model& model, ComPtr<ID3D12GraphicsCommandList> commandList);
	extern int LoadTestScene(std::wstring filename, Model& model, ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void SolveMeshs(tinygltf::Model& tinyModel, int meshIndex, Scene::Model& model, Scene::Node& node, DirectX::XMMATRIX& localToObject);
	extern void SolveNodes(tinygltf::Model& tinyModel, int nodeIndex, Scene::Model& model, DirectX::XMMATRIX& localToObject);
	extern ComPtr<ID3D12Resource> vertexPosUploader;
	extern ComPtr<ID3D12Resource> vertexNormalUploader;
	extern ComPtr<ID3D12Resource> vertexTexCordUploader;
	extern ComPtr<ID3D12Resource> vertexTangentUploader;
	extern ComPtr<ID3D12Resource> vertexColorUploader;
	extern ComPtr<ID3D12Resource> indexUploader;
	extern Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData,
		UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
}