#pragma once



namespace Config {
	//Graphics Related
	const int frameCount = 3;
	const bool RAYTRACE = true;
	const bool useGPUBasedValidation = true;
	const std::wstring gltfFileDirectory  = L"Models\\BoomBox\\glTF";
	const std::wstring gltfFileName = L"BoomBox.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\testscene";
	//const std::wstring gltfFileName = L"testscene1.gltf";
	const std::wstring testSceneFilePath = L"Models\\skull.txt";
	const std::wstring testShaderPath = L"Shaders\\testshader.hlsl";
	const std::wstring shaderFilePath = L"Shaders\\shader.hlsl";
	const bool WIREFRAME_MODE = false;
}