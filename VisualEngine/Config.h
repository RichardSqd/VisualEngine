#pragma once



namespace Config {
	//Graphics Related
	const int numFrameResource = 3;
	const int numRenderTargets = 3;
	const bool RAYTRACE = true;
	const bool useGPUBasedValidation = true;

	const std::wstring gltfFileDirectory  = L"Models\\Sponza\\glTF";
	const std::wstring gltfFileName = L"Sponza.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\Fox\\glTF";
	//const std::wstring gltfFileName = L"Fox.gltf";
	
	// SciFiHelmet
	//const std::wstring gltfFileDirectory  = L"Models\\SciFiHelmet\\glTF";
	//const std::wstring gltfFileName = L"SciFiHelmet.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\SciFiHelmet\\glTF";
	// BoomBox 
	//const std::wstring gltfFileName = L"\Suzanne.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\\Suzanne\\glTF";
	// Buggy
	//const std::wstring gltfFileDirectory  = L"Models\\Buggy\\glTF";
	//const std::wstring gltfFileName = L"Buggy.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\testscene";
	//const std::wstring gltfFileName = L"testscene44.gltf";
	//const std::wstring gltfFileName = L"testscene16.gltf";
	const std::wstring testSceneFilePath = L"Models\\skull.txt";
	const std::wstring testShaderPath = L"Shaders\\testshader.hlsl";
	const std::wstring shaderFilePath = L"Shaders\\shader.hlsl";
	const bool WIREFRAME_MODE = false;
}