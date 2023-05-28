#pragma once



namespace Config {
	//Graphics Related
	const int numFrameResource = 3;
	const int numRenderTargets = 3;
	const bool RAYTRACE = false;
	const bool useGPUBasedValidation = true;
	const int NUMCONTEXTS = 4;
	//Sponza
	//const std::wstring gltfFileDirectory  = L"Models\\Sponza\\glTF";
	//const std::wstring gltfFileName = L"Sponza.gltf";
	
	//const std::wstring gltfFileDirectory  = L"Models\\Fox\\glTF";
	//const std::wstring gltfFileName = L"Fox.gltf";
	
	// SciFiHelmet
	//const std::wstring gltfFileDirectory  = L"Models\\SciFiHelmet\\glTF";
	//const std::wstring gltfFileName = L"SciFiHelmet.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\SciFiHelmet\\glTF";
	// BoomBox 
	//const std::wstring gltfFileName = L"\BoomBox.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\\BoomBox\\glTF";
	//Suzanne
	//const std::wstring gltfFileName = L"\Suzanne.gltf";
	//const std::wstring gltfFileDirectory  = L"Models\\\Suzanne\\glTF";
	// Buggy
	//const std::wstring gltfFileDirectory  = L"Models\\Buggy\\glTF";
	//const std::wstring gltfFileName = L"Buggy.gltf";
	
	const std::wstring gltfFileDirectory  = L"Models\\testscene";
	//const std::wstring gltfFileName = L"minimum_animation.gltf";
	//const std::wstring gltfFileName = L"animated_translated_cube.gltf";
	//const std::wstring gltfFileName = L"animated_translated_objects.gltf";
	//const std::wstring gltfFileName = L"balckb.gltf";
	//const std::wstring gltfFileName = L"test_board.gltf";
	//const std::wstring gltfFileName = L"chess_board_static.gltf";
	//const std::wstring gltfFileName = L"chess_board_static2.gltf";

	const std::wstring gltfFileName = L"animated_chess_animated6.gltf";
	//const std::wstring gltfFileName = L"rougnness_test.gltf";
	//const std::wstring gltfFileName = L"animated_cube.gltf";
	//const std::wstring gltfFileName = L"testscene16.gltf";


	//hlsl
	const std::wstring testSceneFilePath = L"Models\\skull.txt";
	const std::wstring testShaderPath = L"Shaders\\testshader.hlsl";

	const std::wstring defaultVertexShaderFilePath = L"Shaders\\defaultVS.hlsl";
	const std::wstring defaultPixelShaderFilePath = L"Shaders\\defaultPhongPS.hlsl";
	const std::wstring defaultPBRPixelShaderFilePath = L"Shaders\\defaultPS_PBR.hlsl";

	const std::wstring cubemapVertexShaderFilePath = L"Shaders\\cubemapVS.hlsl";
	const std::wstring cubemapPixelShaderFilePath = L"Shaders\\cubemapPS.hlsl";

	const std::wstring skyboxVertexShaderFilePath = L"Shaders\\skyboxVS.hlsl";
	const std::wstring skyboxPixelShaderFilePath = L"Shaders\\skyboxPS.hlsl";

	const std::wstring irradianceVertexShaderFilePath = L"Shaders\\irradianceVS.hlsl";
	const std::wstring irradiancePixelShaderFilePath = L"Shaders\\irradiancePS.hlsl";

	const std::wstring shadowMapPixelShaderFilePath = L"Shaders\\shadowMapPS.hlsl";

	//IBL image 
	//const std::wstring iblImagePath = L"Models\\\HDR\\brown_photostudio_05_2k.hdr";
	const std::wstring iblImagePath = L"Models\\HDR\\blue_photo_studio_2k.hdr";
	//const std::wstring iblImagePath = L"Models\\\HDR\\lilienstein_2k.hdr";
	
	
}