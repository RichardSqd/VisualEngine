#pragma once

namespace Config {
	//Graphics Related
	const bool RAYTRACE = false;
	const bool useGPUBasedValidation = true;
	const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	const std::wstring gltfFilePath = L"Models\\Avocado\\glTF\\Avocado.gltf";
	const std::wstring shaderFilePath = L"Shaders\\shader.hlsl";
}