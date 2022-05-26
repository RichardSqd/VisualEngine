#pragma once
#include "Model.h"
#include "tiny_gltf.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

namespace Scene {
	extern int translate(tinygltf::Model& tinyModel, Scene::Model& model);
	extern int LoadScene(std::wstring filename, Model& model);
	extern void SolveMeshs(tinygltf::Model& tinyModel, int meshIndex, Scene::Model& model, DirectX::XMMATRIX& localToObject);
	extern void SolveNodes(tinygltf::Model& tinyModel, int nodeIndex, Scene::Model& model, DirectX::XMMATRIX& localToObject);
}