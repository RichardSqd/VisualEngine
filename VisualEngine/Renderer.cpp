#include "pch.h"
#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include "Control.h"
#include "imgui_impl_dx12.h"
#include "ShaderLightingData.h"
#include "MultithreadingContext.h"
#include "Cubemap.h"

#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "D3D12.lib")
//#pragma comment(lib, "dxgi.lib") 
//#pragma comment(lib, "dxguid.lib")

using namespace DirectX;
namespace Renderer {
	
	ComPtr<ID3D12RootSignature> rRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> rCubemapRootSignature = nullptr;
	ComPtr<ID3DBlob> rVertexShader = nullptr;
	ComPtr<ID3DBlob> rPixelShader = nullptr;
	ComPtr<ID3DBlob> rPixelPBRShader = nullptr;
	ComPtr<ID3DBlob> rCubemapVertexShader = nullptr;
	ComPtr<ID3DBlob> rCubemapPixelShader = nullptr;
	ComPtr<ID3DBlob> rIrradianceVertexShader = nullptr;
	ComPtr<ID3DBlob> rIrradiancePixelShader = nullptr;
	ComPtr<ID3DBlob> rSkyboxVertexShader = nullptr;
	ComPtr<ID3DBlob> rSkyboxPixelShader = nullptr;
	ComPtr<ID3DBlob> rShadowMapPixelShader = nullptr;

	ComPtr<ID3D12PipelineState> rPso = nullptr;
	ComPtr<ID3D12PipelineState> rPsoShadow = nullptr;
	ComPtr<ID3D12PipelineState> rPsoWireframe = nullptr;
	ComPtr<ID3D12PipelineState> rPsoPBR = nullptr;
	ComPtr<ID3D12PipelineState> rPsoCubemap = nullptr;
	ComPtr<ID3D12PipelineState> rPsoIrradiance = nullptr;
	ComPtr<ID3D12PipelineState> rPsoSkybox = nullptr;
	ComPtr<ID3D12GraphicsCommandList> rCommandList = nullptr;
	ComPtr<ID3D12CommandAllocator> rCommandAlloc = nullptr;
	D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc {};
	D3D12_INPUT_LAYOUT_DESC rCubemapInputLayoutDesc{};
	CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc {};
	
	ComPtr<ID3D12Resource> rRenderTargetBuffer[Config::numRenderTargets];
	ComPtr<ID3D12Resource> rDepthStencilBuffer;
	ComPtr<ID3D12Resource> rCubemapDepthStencilBuffer;
	ComPtr<ID3D12Resource> rSkyboxArray;
	ComPtr<ID3D12Resource> rIrradianceSkyboxArray;

	D3D12_VIEWPORT gScreenViewport {};
	INT gCurBackBufferIndex = 0;

	DirectX::XMFLOAT4X4 gview{};

	Camera gMainCam{};

	//UI interactable parameters
	int rframeRateCap60 = 1;
	int shaderSelector = 0;
	bool wireframeMode = 0;
	bool animationEnabled = false;

	UINT hdriImageCBVHeapIndexStart = 0;
	UINT cubemapIndividualSRVHeapIndexStart = 0;
	UINT cubemappassCBVHeapIndexStart = 0;
	UINT cubemapSRVHeapIndexStart = 0;
	UINT objectCBVHeapIndexStart = 0;
	UINT passCBVHeapIndexStart = 0;
	UINT shadowMapSRVHeapIndexStart = 0;
	UINT shadowmapCBVHeapIndexStart = 0;
	UINT lightCBVHeapIndexStart = 0;
	UINT matCBVHeapIndexStart = 0;
	UINT texSRVHeapIndexStart = 0;
	UINT guiSRVHeapIndexStart = 0;
	
	
	bool runAtIFirstIteration = true;



	void Init(CommandContext* context) {
		rCommandList = context->getCommandList();
		rCommandAlloc = context->getCommandAllocator();
		std::wstring filepath = Config::gltfFileDirectory + L"\\" + Config::gltfFileName;
		Scene::LoadIBLImage(rCommandList, EngineCore::eModel);
		ASSERT(Scene::LoadScene(filepath, EngineCore::eModel, rCommandList));
		InitCamera();
		CreateSwapChain();
		CreateDescriptorHeaps();
		CreateShadersAndInputLayout();
		CreateRootSigniture();
		CreatePipelineState();	
		CreateFrameResources();
		CreateCubemapResources();
		CreateShadowMapDsvs();
		CreateConstantBufferViews();
		CreateShaderResourceViews();
		
		
	}

	void InitCamera() {
		gMainCam.camPhi =  DirectX::XM_PIDIV4;
		gMainCam.camTheta =  1.5f * DirectX::XM_PI;
		gMainCam.camRadius = 5.0f;
		XMStoreFloat4x4(&gMainCam.proj,Math::IdentityMatrix());


	}

	void CreateSwapChain() {
		ComPtr<IDXGISwapChain1> swapChain;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = (UINT) Graphics::gWidth;
		swapChainDesc.Height = (UINT)Graphics::gHeight;
		swapChainDesc.Format = Graphics::gBackBufferFormat;
		swapChainDesc.BufferCount = Graphics::gSwapChainBufferCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		//no msaa for now 
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		//DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		//fsSwapChainDesc.Windowed = TRUE;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Stereo = 0;
		BREAKIFFAILED(Graphics::gdxgiFactory->CreateSwapChainForHwnd(
			Graphics::gCommandQueueManager.GetGraphicsQueue().GetQueue().Get(),
			Graphics::ghWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		));

		BREAKIFFAILED(swapChain.As(&Graphics::gSwapChain));
		BREAKIFFAILED(Graphics::gdxgiFactory->MakeWindowAssociation(Graphics::ghWnd, DXGI_MWA_NO_ALT_ENTER));
		
		
		Graphics::gFrameIndex = Graphics::gSwapChain->GetCurrentBackBufferIndex();

	}



	void CreateRootSigniture() {
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		BREAKIFFAILED(Graphics::gDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)));

		CD3DX12_DESCRIPTOR_RANGE1 ranges[7] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //diffuse,metalicroughness, normal, AO textures, emmision, register t1,2,3 
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //object constant buffer
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //global constant buffer
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //mat constant buffer
		ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //light constant buffer
		ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //shadow texture
		ranges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 6, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //diffuse & specular ibl cubemap 
		//ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[7] = {};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[5].InitAsDescriptorTable(1, &ranges[5], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[6].InitAsDescriptorTable(1, &ranges[6], D3D12_SHADER_VISIBILITY_PIXEL);
		//rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_DESCRIPTOR_RANGE1 cubemapRanges[2] = {};
		cubemapRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //hdr texture
		cubemapRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //global constant buffer

		CD3DX12_ROOT_PARAMETER1 cubemapRootParameters[2] = {};
		cubemapRootParameters[0].InitAsDescriptorTable(1, &cubemapRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		cubemapRootParameters[1].InitAsDescriptorTable(1, &cubemapRanges[1], D3D12_SHADER_VISIBILITY_ALL);



		auto staticSamplers = GetStaticSamplers();

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> serializedRootSig;
		ComPtr<ID3DBlob> err;
		BREAKIFFAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &serializedRootSig, &err));
		if (err != nullptr && err->GetBufferSize() > 0) {
			Utils::Print((char*)err->GetBufferPointer());
		}
		
		BREAKIFFAILED(Graphics::gDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(rRootSignature.GetAddressOf())));
	
	
		//cubemap & skybox
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC cubemapRootSignatureDesc = {};
		ComPtr<ID3DBlob> cubemapSerializedRootSig;
		cubemapRootSignatureDesc.Init_1_1(_countof(cubemapRootParameters), cubemapRootParameters, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		BREAKIFFAILED(D3DX12SerializeVersionedRootSignature(&cubemapRootSignatureDesc, featureData.HighestVersion, &cubemapSerializedRootSig, &err));
		if (err != nullptr && err->GetBufferSize() > 0) {
			Utils::Print((char*)err->GetBufferPointer());
		}
		BREAKIFFAILED(Graphics::gDevice->CreateRootSignature(0, cubemapSerializedRootSig->GetBufferPointer(), cubemapSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(rCubemapRootSignature.GetAddressOf())));

	
	}

	

	void CreateShadersAndInputLayout() {
#if defined(DEBUG) || defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
		ComPtr<ID3DBlob> errors;
		BREAKIFFAILED(D3DCompileFromFile(Config::defaultVertexShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &rVertexShader, &errors));

		if (errors!=nullptr && errors->GetBufferSize()>0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}


		D3DCompileFromFile(Config::defaultPBRPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rPixelPBRShader, &errors);
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		BREAKIFFAILED(D3DCompileFromFile(Config::defaultPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rPixelShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		//hdr to cubemap hlsl
		BREAKIFFAILED(D3DCompileFromFile(Config::cubemapVertexShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &rCubemapVertexShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}


		BREAKIFFAILED(D3DCompileFromFile(Config::cubemapPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rCubemapPixelShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		//irradiance hlsl 
		BREAKIFFAILED(D3DCompileFromFile(Config::irradianceVertexShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &rIrradianceVertexShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		BREAKIFFAILED(D3DCompileFromFile(Config::irradiancePixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rIrradiancePixelShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		//skybox shader
		BREAKIFFAILED(D3DCompileFromFile(Config::skyboxVertexShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &rSkyboxVertexShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}


		BREAKIFFAILED(D3DCompileFromFile(Config::skyboxPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rSkyboxPixelShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}

		//shadowMap shader 
		BREAKIFFAILED(D3DCompileFromFile(Config::shadowMapPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rShadowMapPixelShader, &errors));
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}
		

		rInputLayoutDesc.pInputElementDescs = Scene::inputLayoutDesc;
		rInputLayoutDesc.NumElements = _countof(Scene::inputLayoutDesc);

		rCubemapInputLayoutDesc.pInputElementDescs = Scene::cubeMapinputLayoutDesc;
		rCubemapInputLayoutDesc.NumElements = _countof(Scene::cubeMapinputLayoutDesc);

		//rskyboxInputLayoutDesc.pInputElementDescs = Scene::cubeMapinputLayoutDesc;
		//rskyboxInputLayoutDesc.NumElements = _countof(Scene::cubeMapinputLayoutDesc);
	}

	
	void CreatePipelineState() {

		//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		//msQualityLevels.Format = mBackBufferFormat;
		//msQualityLevels.SampleCount = 4;
		//msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		//msQualityLevels.NumQualityLevels = 0;
		//ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		//	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		//	&msQualityLevels,
		//	sizeof(msQualityLevels)));

		//m4xMsaaQuality = msQualityLevels.NumQualityLevels;

		CD3DX12_DEPTH_STENCIL_DESC DepthStateDisabled = {};
		DepthStateDisabled.DepthEnable = FALSE;
		DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStateDisabled.StencilEnable = FALSE;
		DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateDisabled.BackFace = DepthStateDisabled.FrontFace;

		CD3DX12_DEPTH_STENCIL_DESC DepthStateReadWrite = DepthStateDisabled;
		DepthStateReadWrite.DepthEnable = TRUE;
		DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

		CD3DX12_DEPTH_STENCIL_DESC DepthStateReadOnly = DepthStateReadWrite;
		DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;


		rDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		//int n = (int)rVertexShader->GetBufferSize();
		//Utils::Print("\nVertex Shader size is:");
		//Utils::Print(std::to_string(n).c_str());
		//Utils::Print("\n");
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = rInputLayoutDesc;
		//psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
		psoDesc.pRootSignature = rRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(rVertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(rPixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); 
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = Graphics::gBackBufferFormat;
		psoDesc.DSVFormat = Graphics::gDepthStencilFormat; // DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1; //no msaa for now 

		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPso)));


		//wireframe
		auto wireframePsoDes = psoDesc;
		wireframePsoDes.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&wireframePsoDes, IID_PPV_ARGS(&rPsoWireframe)));

		//pbr
		auto pbrPsoDes = psoDesc;
		pbrPsoDes.PS = CD3DX12_SHADER_BYTECODE(rPixelPBRShader.Get());
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&pbrPsoDes, IID_PPV_ARGS(&rPsoPBR)));


		//create cubemap pso
		auto cubemapPsoDes = psoDesc;
		cubemapPsoDes.InputLayout = rCubemapInputLayoutDesc;
		cubemapPsoDes.pRootSignature = rCubemapRootSignature.Get();
		cubemapPsoDes.VS = CD3DX12_SHADER_BYTECODE(rCubemapVertexShader.Get());
		cubemapPsoDes.PS = CD3DX12_SHADER_BYTECODE(rCubemapPixelShader.Get());

		cubemapPsoDes.NumRenderTargets = 1;
		cubemapPsoDes.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		cubemapPsoDes.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&cubemapPsoDes, IID_PPV_ARGS(&rPsoCubemap)));

		//create irradiance pso
		auto irradiancePsoDes = cubemapPsoDes;
		irradiancePsoDes.VS = CD3DX12_SHADER_BYTECODE(rIrradianceVertexShader.Get());
		irradiancePsoDes.PS = CD3DX12_SHADER_BYTECODE(rIrradiancePixelShader.Get());
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&irradiancePsoDes, IID_PPV_ARGS(&rPsoIrradiance)));

		//skybox pso
		auto skyboxPsoDes = cubemapPsoDes;
		skyboxPsoDes.InputLayout.NumElements = 0;
		skyboxPsoDes.InputLayout.pInputElementDescs = nullptr;
		skyboxPsoDes.pRootSignature = rCubemapRootSignature.Get();
		//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		
		//psoDesc.DepthStencilState.DepthEnable = true;
		//psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		//psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//.DepthStencilState.StencilEnable = false;
		skyboxPsoDes.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		skyboxPsoDes.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		skyboxPsoDes.VS = CD3DX12_SHADER_BYTECODE(rSkyboxVertexShader.Get());
		skyboxPsoDes.PS = CD3DX12_SHADER_BYTECODE(rSkyboxPixelShader.Get());
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&skyboxPsoDes, IID_PPV_ARGS(&rPsoSkybox)));


		//shadow map pso
		auto shadowPsoDes = psoDesc;
		shadowPsoDes.PS = CD3DX12_SHADER_BYTECODE(0,0);
		shadowPsoDes.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		shadowPsoDes.NumRenderTargets = 0;
		shadowPsoDes.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		shadowPsoDes.DepthStencilState.DepthEnable = true;
		shadowPsoDes.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		shadowPsoDes.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		shadowPsoDes.DepthStencilState.StencilEnable = FALSE;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&shadowPsoDes, IID_PPV_ARGS(&rPsoShadow)));

	}

	void UpdatePipelineState() {

	}

	void CreateFrameResources() {
		Graphics::gFrameResourceManager.CreateFrameResources(Graphics::gNumFrameResources);
		
	}

	void CreateShadowMapDsvs() {
	
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(2, Graphics::gDSVDescriptorSize);

		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {

			auto& shadowMap = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->shadowMap;
			//create shadow map dsv 
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			Graphics::gDevice->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, dsvHandle);
			dsvHandle.Offset(1, Graphics::gDSVDescriptorSize);
		}
	}

	
	

	void CreateConstantBufferViews() {
		auto& model = EngineCore::eModel;
		UINT objectCBByteSize = Graphics::gObjectCBByteSize;
		UINT primCount = model.numPrimitives;

		//hdr map srv
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

			int heapIndex = 0;
			auto& texResource = model.textures["IBL_Texture"]->textureResource;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = texResource->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
			Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);

			
		}

		objectCBVHeapIndexStart = cubemapSRVHeapIndexStart + 2;//cubemap as irradiance & specular srvs


		//generate constant buffer views for all frame resources 
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto objCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->objCB->GetResource();

			for (UINT nodeIndex = 0; nodeIndex < model.numNodes; nodeIndex++) {
				auto& node = model.nodes[nodeIndex];
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objCB->GetGPUVirtualAddress();

				//offset to each object constant buffer
				cbAddress += static_cast<unsigned long long>(nodeIndex) * objectCBByteSize;

				//offset to the object cbv in the descriptor heap 
				int heapIndex = objectCBVHeapIndexStart + frameIndex * model.numNodes + nodeIndex;
				node.cbdHeapIndexByFrames[frameIndex] = heapIndex;
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
			
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = objectCBByteSize;
			
				Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			}

		}
		passCBVHeapIndexStart = objectCBVHeapIndexStart + model.numNodes * Graphics::gNumFrameResources;

		UINT passCBByteSize = Graphics::gPassCBByteSize;
		//pass CBVs 
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto passCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->passCB->GetResource();

			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

			//offset to each pass constant buffer
			int heapIndex = frameIndex + passCBVHeapIndexStart;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);


			//Utils::Print(std::to_string(frameIndex).c_str());
			//Utils::Print(std::to_string(heapIndex).c_str());

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = passCBByteSize;
			Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
		}

		shadowMapSRVHeapIndexStart = passCBVHeapIndexStart + Graphics::gNumFrameResources;
		//shadow map srv for each frame resource
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {


			int heapIndex = frameIndex + shadowMapSRVHeapIndexStart;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.Texture2D.MipLevels = 1;
			Graphics::gDevice->CreateShaderResourceView(Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->shadowMap.Get(), &srvDesc, handle);

		}

		shadowmapCBVHeapIndexStart = shadowMapSRVHeapIndexStart + Graphics::gNumFrameResources;
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto shadowCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->shadowCB->GetResource();

			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = shadowCB->GetGPUVirtualAddress();

			//offset to each pass constant buffer
			int heapIndex = frameIndex + shadowmapCBVHeapIndexStart;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);


			//Utils::Print(std::to_string(frameIndex).c_str());
			//Utils::Print(std::to_string(heapIndex).c_str());

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = Graphics::gShadowCBByteSize;
			Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
		}

		
		lightCBVHeapIndexStart = shadowmapCBVHeapIndexStart + Graphics::gNumFrameResources;
		UINT lightCBByteSize = Graphics::gLightCBByteSize;
		auto& sceneLighting = EngineCore::eModel.lights;
		UINT lightCount = (UINT)sceneLighting.numDirectionalLights + sceneLighting.numPointLights + sceneLighting.numSpotLights;
		//create light views
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto lightCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->lightCB->GetResource();
			{
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = lightCB->GetGPUVirtualAddress();
				
				//offset to the object cbv in the descriptor heap 
				int heapIndex = lightCBVHeapIndexStart + frameIndex;


				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = lightCBByteSize;
				Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			}
		}
		

		matCBVHeapIndexStart = lightCBVHeapIndexStart + 1 * Graphics::gNumFrameResources;

		UINT matCBByteSize = Graphics::gMatCBByteSize;
		UINT matCount = (UINT)EngineCore::eModel.materials.size();

		//create material views
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto matCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->matCB->GetResource();
			for (UINT matIndex = 0; matIndex < matCount; matIndex++) {
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = matCB->GetGPUVirtualAddress();
				//offset to each mat const buffer 
				cbAddress += static_cast<unsigned long long>(matIndex) * matCBByteSize;

				//offset to the object cbv in the descriptor heap 
				int heapIndex = matCBVHeapIndexStart +
								frameIndex * matCount + matIndex;
				//Utils::Print(std::to_wstring(matCount).c_str());
				//Utils::Print(std::to_wstring(heapIndex).c_str());
				
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = matCBByteSize;
				Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			}

		}
		texSRVHeapIndexStart = matCBVHeapIndexStart + matCount * Graphics::gNumFrameResources;

	}


	void CreateShaderResourceViews() {
		//create texture heap descriptors and associate them with loaded resources
		
		UINT matCount = (UINT)EngineCore::eModel.materials.size();
		UINT textureCount = (UINT)EngineCore::eModel.textures.size();
		auto& model = EngineCore::eModel;
		auto& textures = EngineCore::eModel.textures;
		
		

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		int heapIndex = texSRVHeapIndexStart;

		for (UINT i = 0; i < model.materials.size(); i++) {
			auto& mat = model.materials[i];
			if (mat->hasDiffuseTexture) {
				mat->diffuseMapSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texDiffuseMap]->textureResource;
				auto des = texResource->GetDesc();
				srvDesc.Format = des.Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
			}
			else {
				mat->diffuseMapSrvHeapIndex = heapIndex;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
			}
			heapIndex++;

			if (mat->hasMetallicRoughnessTexture) {
				mat->roughnessMetallicMapSrvHeaIndex = heapIndex;
				auto& texResource = textures[mat->texroughnessMetallicMap]->textureResource;
				auto des = texResource->GetDesc();
				srvDesc.Format = des.Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
				
			}
			else {
				mat->roughnessMetallicMapSrvHeaIndex = heapIndex;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
			}
			heapIndex++;

			if (mat->hasNormalTexture) {
				mat->normalMapSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texNormalMap]->textureResource;
				srvDesc.Format = texResource->GetDesc().Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
				
			}
			else {
				mat->normalMapSrvHeapIndex = heapIndex;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
			}

			heapIndex++;
			if (mat->hasOcclusionTexture) {
				mat->occlusionMapSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texOcclusionMap]->textureResource;
				srvDesc.Format = texResource->GetDesc().Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);


			}
			else {
				mat->occlusionMapSrvHeapIndex = heapIndex;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
			}

			heapIndex++;
			if (mat->hasEmissiveTexture) {
				mat->emissiveSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texEmisiveMap]->textureResource;
				srvDesc.Format = texResource->GetDesc().Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
			}
			else {
				mat->emissiveSrvHeapIndex = heapIndex;
				CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);
			}



			heapIndex++;

		}


		guiSRVHeapIndexStart = heapIndex;

	}

	void CreateDescriptorHeaps() {
		//create rtv desc heap 
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors = 
				Graphics::gSwapChainBufferCount 
				+ 6 //hdr cube map render target views
				+ 6;//irradiance render target views
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(Graphics::gRtvHeap.GetAddressOf())));

		//create dsv desc heap (one dsv for each frame buffers and one for the scene)
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors =  1 
									+ 1 //cubemap
									+ Graphics::gNumFrameResources;//shadow map
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(Graphics::gDsvHeap.GetAddressOf())));

		//create srv/cbv desc heap 
		UINT numDescriptors = (static_cast<unsigned long long>(EngineCore::eModel.numNodes) 
			+ 1 //skybox
			+ 6 //cubemap faces as seperate texture resources
			+ 6 //irradiance maps
			+ 6 //cubemap global parameters 
			+ 2 //cubemap srv irradiance & specular
			+ (UINT)EngineCore::eModel.materials.size()) * Graphics::gNumFrameResources //materials 
			+ static_cast<unsigned long long>((UINT)EngineCore::eModel.materials.size()) * 5 //material textures
			+ 1 * static_cast<unsigned long long>(Graphics::gNumFrameResources) //pass cbv
			+ 1 * static_cast<unsigned long long>(Graphics::gNumFrameResources) //shadow maps for each frame resource
			+ 1 //light 
			+ 1; //gui
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc{};
		cbvSrvHeapDesc.NumDescriptors = numDescriptors;
		cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvSrvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(Graphics::gCbvSrvHeap.GetAddressOf())));

		//create sampler desc heap 
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{};
		samplerHeapDesc.NumDescriptors = 2048;
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		samplerHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(Graphics::gSamplerHeap.GetAddressOf())));

	}

	void OnResize() {
		ASSERT(Graphics::gDevice);
		ASSERT(Graphics::gSwapChain);
		ASSERT(Renderer::rCommandAlloc);
		auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();

		queue.FlushCommandQueue();
		BREAKIFFAILED(rCommandList->Reset(Renderer::rCommandAlloc.Get(), nullptr));

		for (UINT i = 0; i < Graphics::gSwapChainBufferCount; i++) {
			rRenderTargetBuffer[i].Reset();
		}
		rDepthStencilBuffer.Reset();

		BREAKIFFAILED(Graphics::gSwapChain->ResizeBuffers(Graphics::gSwapChainBufferCount,
								(UINT)Graphics::gWidth, (UINT)Graphics::gHeight,
								Graphics::gBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		gCurBackBufferIndex = 0;

		//create rtv on heap for render target buffers 
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < Graphics::gSwapChainBufferCount; i++) {

			BREAKIFFAILED(Graphics::gSwapChain->GetBuffer(i,IID_PPV_ARGS(&rRenderTargetBuffer[i])));
			Graphics::gDevice->CreateRenderTargetView(rRenderTargetBuffer[i].Get(), nullptr, rtvHeapHandle);

			rtvHeapHandle.Offset(1, Graphics::gRTVDescriptorSize);
		}

		//Create DS buffer and view 
		D3D12_RESOURCE_DESC depthStencilDesc {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = (UINT)Graphics::gWidth;
		depthStencilDesc.Height = (UINT)Graphics::gHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = 1;//msaa
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear {};
		optClear.Format = Graphics::gDepthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(rDepthStencilBuffer.GetAddressOf()));
		rDepthStencilBuffer->SetName(L"DepthStencilBuffer");

		//create dsv
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = Graphics::gDepthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		Graphics::gDevice->CreateDepthStencilView(rDepthStencilBuffer.Get(), &dsvDesc, Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());

		//state transition to STATE_DEPTH_WRITE
		rCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		//Resize
		BREAKIFFAILED(rCommandList->Close());
		ID3D12CommandList* cmdLsts[] = { rCommandList.Get() };
		queue.GetQueue()->ExecuteCommandLists(_countof(cmdLsts), cmdLsts);

		queue.FlushCommandQueue();

		Graphics::gScreenViewport.TopLeftX = 0;
		Graphics::gScreenViewport.TopLeftY = 0;
		gScreenViewport.Width = Graphics::gWidth;
		gScreenViewport.Height = Graphics::gHeight;
		gScreenViewport.MinDepth = 0.0f;
		gScreenViewport.MaxDepth = 1.0f;

		Graphics::gScissorRect = { 0,0,static_cast<int>(Graphics::gWidth), static_cast<int>(Graphics::gHeight) };

		//update the projection matrix after the aspect ratio has been changed
		
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * Math::PI, Graphics::AspectRatio(), 0.1f, 1000.0f);
		DirectX::XMMATRIX P2 = DirectX::XMMatrixPerspectiveFovRH(0.25f * Math::PI, Graphics::AspectRatio(), 0.1f, 100.0f);

		
		//DirectX::XMFLOAT4X4 X;
		//DirectX::XMStoreFloat4x4(&X, M);
		
		DirectX::XMStoreFloat4x4(&gMainCam.proj, P2);
	


	}

	void CreateCubemapResources() {
		
		//create the texture 
		UINT width = 1024;
		UINT height = 1024;
		auto& model = EngineCore::eModel;
		auto& skyboxHDRTexture = model.textures["IBL_Texture"];


		D3D12_RESOURCE_DESC texArrayDesc = {};
		texArrayDesc.MipLevels = 1;
		texArrayDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texArrayDesc.Alignment = 0;
		texArrayDesc.Width = width;
		texArrayDesc.Height = height;
		texArrayDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		texArrayDesc.DepthOrArraySize = 6;
		texArrayDesc.SampleDesc.Count = 1;
		texArrayDesc.SampleDesc.Quality = 0;
		texArrayDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = 1.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		//create irradiance/specular cubemap resource
		Graphics::gDevice.Get()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texArrayDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(rSkyboxArray.ReleaseAndGetAddressOf()));

		Graphics::gDevice.Get()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texArrayDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(rIrradianceSkyboxArray.ReleaseAndGetAddressOf()));

		int cbvhandleIndex = 0;
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
		cbvhandleIndex += 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(Graphics::gSwapChainBufferCount, Graphics::gRTVDescriptorSize);


		//create dsv
		D3D12_RESOURCE_DESC depthStencilDesc {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = 1;//msaa
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear {};
		optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(rCubemapDepthStencilBuffer.GetAddressOf()));
		rCubemapDepthStencilBuffer->SetName(L"DepthStencilBuffer");

		//create dsv
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Texture2D.MipSlice = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(1, Graphics::gDSVDescriptorSize);

		
		Graphics::gDevice->CreateDepthStencilView(rCubemapDepthStencilBuffer.Get(), &dsvDesc, dsvHandle);

		cubemapIndividualSRVHeapIndexStart = cbvhandleIndex;
		for (int i = 0; i < 6; i++) {

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = 1;
			srvDesc.Texture2DArray.FirstArraySlice = i;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = texArrayDesc.Format;


			//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			//uavDesc.Format = textureDesc.Format;
			//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			//uavDesc.Texture2D.MipSlice = 0;
		
			
			Graphics::gDevice->CreateShaderResourceView(rSkyboxArray.Get(), &srvDesc, handle);
			handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			cbvhandleIndex += 1;

			D3D12_RENDER_TARGET_VIEW_DESC cubemapImagesRTV = {};
			cubemapImagesRTV.Format = texArrayDesc.Format;
			cubemapImagesRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			cubemapImagesRTV.Texture2DArray.MipSlice = 0;
			cubemapImagesRTV.Texture2DArray.PlaneSlice = 0;
			cubemapImagesRTV.Texture2DArray.ArraySize = 1;
			cubemapImagesRTV.Texture2DArray.FirstArraySlice = i;
			
			
			
			Graphics::gDevice->CreateRenderTargetView(rSkyboxArray.Get(), &cubemapImagesRTV, rtvHandle);
			rtvHandle.Offset(1, Graphics::gRTVDescriptorSize);
			
		}

		for (int i = 0; i < 6; i++) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = 1;
			srvDesc.Texture2DArray.FirstArraySlice = i;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = texArrayDesc.Format;

			Graphics::gDevice->CreateShaderResourceView(rIrradianceSkyboxArray.Get(), &srvDesc, handle);
			handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			cbvhandleIndex += 1;
		}
		
		cubemappassCBVHeapIndexStart = cbvhandleIndex;
		//pass constant buffer view
		for (int i = 0; i < 6; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = Cubemap::passCB[i]->GetResource()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = Graphics::gPassCBByteSize;
			Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			cbvhandleIndex += 1;
		}

		cubemapSRVHeapIndexStart = cbvhandleIndex;
		//cubemap 
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = texArrayDesc.MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = texArrayDesc.Format;
		Graphics::gDevice->CreateShaderResourceView(rSkyboxArray.Get(), &srvDesc, handle);

		//cubemap irradiance 
		handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
		Graphics::gDevice->CreateShaderResourceView(rIrradianceSkyboxArray.Get(), &srvDesc, handle);
		
		//irradiancce rtv
		for (int i = 0; i < 6; i++) {
			D3D12_RENDER_TARGET_VIEW_DESC cubemapImagesRTV = {};
			cubemapImagesRTV.Format = texArrayDesc.Format;
			cubemapImagesRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			cubemapImagesRTV.Texture2DArray.MipSlice = 0;
			cubemapImagesRTV.Texture2DArray.PlaneSlice = 0;
			cubemapImagesRTV.Texture2DArray.ArraySize = 1;
			cubemapImagesRTV.Texture2DArray.FirstArraySlice = i;

			Graphics::gDevice->CreateRenderTargetView(rIrradianceSkyboxArray.Get(), &cubemapImagesRTV, rtvHandle);
			rtvHandle.Offset(1, Graphics::gRTVDescriptorSize);
		}


	}

	void CreateRTVDSV() {
		//ASSERT(Graphics::gDevice);
		//ASSERT(Graphics::gSwapChain);
		//ASSERT(Renderer::rCommandAlloc);

	}

	void Update() {
		UpdateInput();
		UpdateCamera();
		

		//advance the frame index, check the status of GPU completion on current frame resources 
		Graphics::gFrameResourceManager.NextFrameResource();
		FrameResource* currentFrameResource = Graphics::gFrameResourceManager.GetCurrentFrameResource();

		auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();

		//if contex switching is signaled, start the DXR rendering 
		if (Graphics::gRayTraced) {
			//clean up
			queue.FlushCommandQueue();
			//prepare for switching 


			return;
		}
		
		auto frameTargetCompletionValue = currentFrameResource->fence;
		//std::wstring index = L"frame" + std::to_wstring(Graphics::gFrameResourceManager.GetCurrentIndex())+ L"  "+ std::to_wstring(queue.GetCurrentFenceValue()) + +L"\n";
		//Utils::Print(index.c_str());
		if (frameTargetCompletionValue > 0 && queue.GetCompletedFenceValue() < frameTargetCompletionValue) {
			//must wait until the queue has completed its tasks for the current frame 
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
			ASSERT(eventHandle);
			queue.SetEventOnCompletion(frameTargetCompletionValue, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}

		UpdateAnimation();
		UpdateObjCBs(currentFrameResource);
		UpdateMaterialCBs(currentFrameResource);
		UpdatePassCB(currentFrameResource);
		UpdateshadowmapCB(currentFrameResource);
		UpdateLightCBs(currentFrameResource);
	}

	void UpdateAnimation() {
		if (!animationEnabled) {
			return;
		}
		ImGuiIO& io = ImGui::GetIO();
		Time::tAnimationTimer += io.DeltaTime;
		float currentTime = (float)Time::tAnimationTimer;
		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
			auto& node = EngineCore::eModel.nodes[i];
			
			DirectX::XMMATRIX rotate = {};
			DirectX::XMMATRIX scale = {};
			DirectX::XMMATRIX translate = {};

			if (node.animation.hasRotationAnimation && currentTime < node.animation.rotationTime[node.animation.rotationTime.size()-1]) {
				DirectX::XMFLOAT4 currentRotaton = {};
				int previousTimeIndex = 0;
				Utils::Print(std::to_string(currentTime).c_str());
				//Utils::Print("\n");
				for (; previousTimeIndex < node.animation.rotationTime.size()-1; previousTimeIndex++) {
					float previousTime = node.animation.rotationTime[previousTimeIndex];
					int nextTimeIndex = previousTimeIndex + 1;
					float nextTime = node.animation.rotationTime[nextTimeIndex];
					if (currentTime >= previousTime && currentTime < nextTime) {
						//Utils::Print(std::to_string(previousTimeIndex).c_str());

						//Utils::Print("\n");

						auto& previousRotation = node.animation.rotationAnimation[previousTimeIndex];
						auto& nextRotation = node.animation.rotationAnimation[nextTimeIndex];

						float interpolationValue = (currentTime - previousTime) / (nextTime - previousTime);
						Utils::Print("  next time  ");
						Utils::Print(std::to_string(nextTime).c_str());
						Utils::Print("  previous time  ");
						Utils::Print(std::to_string(previousTime).c_str());
						Utils::Print("    ");
						Utils::Print(std::to_string(interpolationValue).c_str());
						//Utils::Print("\n");

						Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.x + interpolationValue * (nextRotation.x - previousRotation.x)).c_str());
						//Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.y + interpolationValue * (nextRotation.y - previousRotation.y)).c_str());
						//Utils::Print("    ");
						Utils::Print(std::to_string(previousRotation.z + interpolationValue * (nextRotation.z - previousRotation.z)).c_str());
						Utils::Print("    ");
						Utils::Print(std::to_string(previousRotation.w + interpolationValue * (nextRotation.w - previousRotation.w)).c_str());
						Utils::Print("\n");

						currentRotaton = {
							 previousRotation.x + interpolationValue * (nextRotation.x - previousRotation.x)
							,previousRotation.y + interpolationValue * (nextRotation.y - previousRotation.y)
							,previousRotation.z + interpolationValue * (nextRotation.z - previousRotation.z)
							,previousRotation.w + interpolationValue * (nextRotation.w - previousRotation.w) };

						break;
					}
					
				}			

				DirectX::XMVECTOR rv = DirectX::XMVectorSet(currentRotaton.x, currentRotaton.y, currentRotaton.z, currentRotaton.w);
				rotate = DirectX::XMMatrixRotationQuaternion(rv);

				//DirectX::XMVECTOR vec = DirectX::XMVectorSet(0, 1, 0, 0);
				//rotate = DirectX::XMMatrixRotationAxis(vec, currentTime);
				//rotate = DirectX::XMMatrixIdentity();
		
				DirectX::XMStoreFloat4x4(&node.animation.rotationChannel, rotate);
				
				node.numFrameDirty = Config::numFrameResource;
			
			}

			if (node.animation.hasTranslationAnimation && currentTime < node.animation.translationTime[node.animation.translationTime.size() - 1]) {
				DirectX::XMFLOAT3 currentTranslation = {};
				int previousTimeIndex = 0;
				Utils::Print(std::to_string(currentTime).c_str());
				//Utils::Print("\n");
				for (; previousTimeIndex < node.animation.translationTime.size() - 1; previousTimeIndex++) {
					float previousTime = node.animation.translationTime[previousTimeIndex];
					int nextTimeIndex = previousTimeIndex + 1;
					float nextTime = node.animation.translationTime[nextTimeIndex];
					if (currentTime >= previousTime && currentTime < nextTime) {
						//Utils::Print(std::to_string(previousTimeIndex).c_str());

						//Utils::Print("\n");

						auto& previousTranslation = node.animation.translationAnimation[previousTimeIndex];
						auto& nextTranslation = node.animation.translationAnimation[nextTimeIndex];

						float interpolationValue = (currentTime - previousTime) / (nextTime - previousTime);
						Utils::Print("  next time  ");
						Utils::Print(std::to_string(nextTime).c_str());
						Utils::Print("  previous time  ");
						Utils::Print(std::to_string(previousTime).c_str());
						Utils::Print("    ");
						Utils::Print(std::to_string(interpolationValue).c_str());
						//Utils::Print("\n");

						Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.x + interpolationValue * (nextRotation.x - previousRotation.x)).c_str());
						//Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.y + interpolationValue * (nextRotation.y - previousRotation.y)).c_str());
						//Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.z + interpolationValue * (nextRotation.z - previousRotation.z)).c_str());
						//Utils::Print("    ");
						//Utils::Print(std::to_string(previousRotation.w + interpolationValue * (nextRotation.w - previousRotation.w)).c_str());
						//Utils::Print("\n");

						currentTranslation = {
							 previousTranslation.x + interpolationValue * (nextTranslation.x - previousTranslation.x)
							,previousTranslation.y + interpolationValue * (nextTranslation.y - previousTranslation.y)
							,previousTranslation.z + interpolationValue * (nextTranslation.z - previousTranslation.z)
							 };

						break;
					}

				}

				translate = DirectX::XMMatrixTranslation((float)currentTranslation.x, (float)currentTranslation.y, (float)currentTranslation.z);

				DirectX::XMStoreFloat4x4(&node.animation.translationChannel, translate);
				
				node.numFrameDirty = Config::numFrameResource;

			}
			
				
		}

	}

	void UpdateInput() {
		ImGuiIO& io = ImGui::GetIO();
		if (GetAsyncKeyState('A') & 0x8000) {
			float dx = 2.50f;
			Renderer::gMainCam.camTheta += (float)0.1* dx;
			Renderer::gMainCam.camPhi = Math::Clamp(Renderer::gMainCam.camPhi, 0.1f, Math::PI - 0.1f);
			Control::lastMousePos.x += (long)dx;
			Renderer::gMainCam.viewDirty = true;
		}
		if (GetAsyncKeyState('D') & 0x8000) {
			float dx = 2.50f;
			Renderer::gMainCam.camTheta -= (float)0.1 * dx;
			Renderer::gMainCam.camPhi = Math::Clamp(Renderer::gMainCam.camPhi, 0.1f, Math::PI - 0.1f);
			Control::lastMousePos.x -= (long)dx;
			Renderer::gMainCam.viewDirty = true;
		}
		
			

		//if (GetAsyncKeyState('S') & 0x8000)
		//	mCamera.Walk(-10.0f * dt);

		//if (GetAsyncKeyState('A') & 0x8000)
		//	mCamera.Strafe(-10.0f * dt);

		//if (GetAsyncKeyState('D') & 0x8000)
		//	mCamera.Strafe(10.0f * dt);
	}

	void UpdateCamera() {
		if (gMainCam.viewDirty) {
			
			gMainCam.SetCamPosSph(gMainCam.camPhi, gMainCam.camTheta, gMainCam.camRadius);
			gMainCam.UpdateViewMatrix();
		}
		
	}
	
	void UpdateObjCBs(FrameResource* currentFrameResource) {
		auto curFrameObjCB = currentFrameResource->objCB.get();
		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
			auto& node = EngineCore::eModel.nodes[i];

			if (node.numFrameDirty > 0) {
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&node.toWorldmatrix);
				
				if (animationEnabled 
					&& (node.animation.hasRotationAnimation || node.animation.hasScalingAnimation || node.animation.hasTranslationAnimation) ) {
					auto scale = DirectX::XMLoadFloat4x4(&node.animation.scalingChannel);
					auto rotate = DirectX::XMLoadFloat4x4(&node.animation.rotationChannel);
					auto translate = DirectX::XMLoadFloat4x4(&node.animation.translationChannel);
					
					world = scale * rotate * translate;		
				}

				EngineCore::eModel.lightnumFrameDirty = Config::numFrameResource;
				
				//DirectX::XMMATRIX animated = DirectX::XMLoadFloat4x4(&node.animatedMatrix);
				DirectX::XMFLOAT4 sceneScaling = Scene::sceneScaling;
				DirectX::XMFLOAT4 sceneTranslation = Scene::sceneTranslation;
				DirectX::XMMATRIX temp = 
					DirectX::XMMatrixScaling(sceneScaling.x, sceneScaling.y, sceneScaling.z) 
					* DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat4(&Scene::axis), Scene::angle) 
					* DirectX::XMMatrixTranslation(sceneTranslation.x, sceneTranslation.y, sceneTranslation.z);
				world = world;//* temp;
				
				ObjectConstants objConsts;
				DirectX::XMStoreFloat4x4(&objConsts.World, DirectX::XMMatrixTranspose(world));
				
				DirectX::XMStoreFloat4x4(&objConsts.WorldIT, DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(world), world));
				
				curFrameObjCB->CopyData(i, &objConsts);

				node.numFrameDirty--;
				
			}
		}
	}

	void UpdateMaterialCBs(FrameResource* currentFrameResource) {
		auto curFrameMatCB = currentFrameResource->matCB.get();
		auto& model = EngineCore::eModel;
		for (UINT i = 0; i < model.materials.size(); i++) {
			auto& mat = model.materials[i];
			if (mat->numFrameDirty > 0) {
				Utils::Print("Update MAT CB LOOP");


				MaterialConstants materialConsts = {};
				materialConsts.diffuseFactor = mat->diffuse;
				materialConsts.metallicFactor = mat->metalness;
				materialConsts.roughnessFactor = mat->roughness;
				materialConsts.hasDiffuseTexture = mat->hasDiffuseTexture;
				materialConsts.hasMetallicRoughnessTexture = mat->hasMetallicRoughnessTexture;
				materialConsts.hasNormalTexture = mat->hasNormalTexture ;
				materialConsts.hasOcclusionTexture =  mat->hasOcclusionTexture;
				materialConsts.hasEmissiveTexture = mat->hasEmissiveTexture;
				curFrameMatCB->CopyData(i, &materialConsts);
				mat->numFrameDirty--;
			}
		}


	}

	void UpdateLightCBs(FrameResource* currentFrameResource) {
		auto curFrameLightCB = currentFrameResource->lightCB.get();
		auto& model = EngineCore::eModel;
		
		if (model.lightnumFrameDirty>0) {
			Utils::Print("Update LIGHT CB LOOP\n");
			LightConstants lc = {};
			lc.lights = model.lights;
			curFrameLightCB->CopyData(0, &lc);
			model.lightnumFrameDirty--;
		}
		
	}


	void UpdatePassCB(FrameResource* currentFrameResource){
		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&gMainCam.view);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&gMainCam.proj);
		DirectX::XMMATRIX viewProj = view * proj;

		
		PassConstants pConsts;
		DirectX::XMStoreFloat4x4(&pConsts.ViewProjMatrix, DirectX::XMMatrixTranspose(viewProj));
		DirectX::XMStoreFloat4x4(&pConsts.ViewMatrix, DirectX::XMMatrixTranspose(view));
		DirectX::XMStoreFloat4x4(&pConsts.ProjMatrix, DirectX::XMMatrixTranspose(proj));
		pConsts.CameraPos = gMainCam.camPos;
		pConsts.NearZ = 1.0;
		pConsts.FarZ = 10.0;
		pConsts.shaderSelector = shaderSelector;

		auto curFramePassCB = currentFrameResource->passCB.get();
		curFramePassCB->CopyData(0, &pConsts);


	}

	void UpdateshadowmapCB(FrameResource* currentFrameResource) {
		
		auto& model = EngineCore::eModel;

		//update shadow maps when needed
		if (model.lightnumFrameDirty == 0) 
			return;
		
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&model.lights.directionalLights[0].lightDirection);

		DirectX::XMVECTOR eye = -2.0f * ShaderLightingData::SCENERANGE * lightDir;
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR at = DirectX::XMVectorZero(); //XMVectorAdd(eye, XMLoadFloat3(&model.lights.directionalLights[0].lightDirection));
		//DirectX::XMVECTOR at = XMVectorSet(0.0f, 8.0f, 0.0f, 0.0f);
		XMMATRIX lightView = XMMatrixLookAtRH(eye, at, up);

		XMFLOAT3 sphereCenterRS;
		XMStoreFloat3(&sphereCenterRS, XMVector3TransformCoord(at, lightView));

		// Ortho frustum in light space encloses scene.
		float l = sphereCenterRS.x - ShaderLightingData::SCENERANGE;
		float b = sphereCenterRS.y - ShaderLightingData::SCENERANGE;
		float n = sphereCenterRS.z - ShaderLightingData::SCENERANGE;
		float r = sphereCenterRS.x + ShaderLightingData::SCENERANGE;
		float t = sphereCenterRS.y + ShaderLightingData::SCENERANGE;
		float f = sphereCenterRS.z + ShaderLightingData::SCENERANGE;

		DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicRH(30, 30, 0.1f, 100.0f);
		//DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovRH(0.5f * Math::PI, Graphics::AspectRatio(), 0.1f, 100.0f);
		//XMMATRIX lightProj = XMMatrixOrthographicOffCenterRH(l, r, b, t, n, f);


		DirectX::XMMATRIX lightViewProj = lightView * proj;

		ShadowConstants shadowConsts;
		DirectX::XMStoreFloat4x4(&shadowConsts.ViewProjMatrix, DirectX::XMMatrixTranspose(lightViewProj));
		DirectX::XMStoreFloat4x4(&shadowConsts.ViewMatrix, DirectX::XMMatrixTranspose(lightView));
		DirectX::XMStoreFloat4x4(&shadowConsts.ProjMatrix, DirectX::XMMatrixTranspose(proj));
		DirectX::XMStoreFloat3(&shadowConsts.CameraPos , eye) ;
		auto curshadowCB = currentFrameResource->shadowCB.get();
		curshadowCB->CopyData(0, &shadowConsts);

		currentFrameResource->shadowMapRenderRequired = true;

		XMMATRIX T = {
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f };

		XMMATRIX shadowTransform = lightViewProj * T;

		auto& lights = model.lights;
		DirectX::XMStoreFloat4x4(&lights.directionalLights[0].shadowTransform, DirectX::XMMatrixTranspose(shadowTransform) );
		
	}


	void Draw() {
		
		auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
		
		//prepare for rendering and proceed to evoking working threads 
		{
			//Reset the worker command context for the current frame, as we know 
			//the last commands in queue has been completed for this frame 
			//for (int i = 0; i < Config::NUMCONTEXTS; i++) {
			//	auto& commandContext = Graphics::gFrameResourceManager.GetCurrentFrameResource()->comandContexts[i];
			//	auto& allocator = commandContext->getCommandAllocator();
			//	auto& commandList = commandContext->getCommandList();
			//	BREAKIFFAILED(allocator->Reset());
			//	BREAKIFFAILED(commandList->Reset(allocator.Get(), rPso.Get()));
			//}

			//Reset the main thread command context 
			//BREAKIFFAILED(rCommandAlloc->Reset());
			//BREAKIFFAILED(rCommandList->Reset(rCommandAlloc.Get(), rPso.Get()));
			//rCommandList->Close();
		}

		auto& context = Graphics::gFrameResourceManager.GetCurrentFrameResource()->comandContexts[0];
		auto allocator = context->getCommandAllocator();
		auto commandList = context->getCommandList();
		BREAKIFFAILED(allocator->Reset());
		BREAKIFFAILED(commandList->Reset(allocator.Get(), rPso.Get()));

		//clear depth stencil for shadow map rendering 
		PopulateCommandList(commandList);
		
		//add the command list to queue
		ID3D12CommandList* cmdsLists[] = { commandList.Get()};
		queue.ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		//Swap back and front buffer 
		BREAKIFFAILED(Graphics::gSwapChain->Present(rframeRateCap60, 0));
		gCurBackBufferIndex = (gCurBackBufferIndex + 1) % Graphics::gSwapChainBufferCount;

		//fence move to new position and signal the new GPU fence value 
		queue.AdvanceFenceValue();
		Graphics::gFrameResourceManager.GetCurrentFrameResource()->fence = queue.GetCurrentFenceValue();
		//std::wstring text = L"frame fence" + std::to_wstring(queue.GetCurrentFenceValue()) + L"  " +L"\n";
		//Utils::Print(text.c_str());
		queue.SignalFencePoint();

	}



	void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> commandList) {

		

		//back buffer transition state from present to render target 
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rRenderTargetBuffer[gCurBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		//clear back buffer and depth buffer
		commandList->ClearRenderTargetView(
			CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex, Graphics::gRTVDescriptorSize),
			DirectX::Colors::LightGray, 0, nullptr);
		commandList->ClearDepthStencilView(
			CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		//set cbvsrv descriptor heap
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);

		RenderSkyBox(commandList);

		RenderShadowMap(commandList);

		RenderColor(commandList);

		RenderUI(commandList);
		

		//back buffer transition state from render target back to present
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rRenderTargetBuffer[gCurBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		//finish populating commandlist
		BREAKIFFAILED(commandList->Close());

	}

	void RenderColor(ComPtr<ID3D12GraphicsCommandList> commandList) {
		

		ID3D12PipelineState* pso;
		if (shaderSelector < 2) {
			pso = rPso.Get();
		}
		else {
			pso = rPsoPBR.Get();
		}
		if (wireframeMode) {
			pso = rPsoWireframe.Get();
		}
		commandList->SetPipelineState(pso);
		commandList->SetGraphicsRootSignature(rRootSignature.Get());

		//set diffuse & specular cubemaps
		auto cubemapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cubemapHandle.Offset(cubemapSRVHeapIndexStart, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(6, cubemapHandle);


		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();

		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCBVHeapIndexStart+curFrameIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);

		//lights handle
		//UINT lightCount = (UINT)EngineCore::eModel.lights.size();

		auto lightCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		int lightCbvIndex = lightCBVHeapIndexStart + curFrameIndex;
		lightCbvHandle.Offset(lightCbvIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(4, lightCbvHandle);

		auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		srvHandle.Offset(shadowMapSRVHeapIndexStart + curFrameIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(5, srvHandle);
		
		commandList->OMSetRenderTargets(1,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex,
				Graphics::gRTVDescriptorSize),
			true,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()));

		DrawRenderItems(commandList);
	}

	
	
	void DrawRenderItems(ComPtr<ID3D12GraphicsCommandList> commandList) {

		Scene::Model& model = EngineCore::eModel;
		//UINT objCount = model.numNodes;
		UINT matCount = model.numMaterials;
		
		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
	
			auto& node = model.nodes[i];
			if (node.mesh < 0 || node.mesh>=(int)model.numMeshes) {
				continue;
			}
			auto& primitives = model.meshes[node.mesh].primitives;


			// set obj CBV in the descritpor heap for each node for the current frame resource
			UINT cbvIndex = node.cbdHeapIndexByFrames[curFrameIndex];
			auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
			cbvHandle.Offset(cbvIndex, Graphics::gCbvSrvUavDescriptorSize);
			commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);


			D3D12_VERTEX_BUFFER_VIEW vbvPos = {}; 
			D3D12_VERTEX_BUFFER_VIEW vbvNormal = {};
			D3D12_VERTEX_BUFFER_VIEW vbvTexCord = {};
			D3D12_VERTEX_BUFFER_VIEW vbvTangent = {};
			D3D12_INDEX_BUFFER_VIEW ibv = {};

			
			vbvPos.StrideInBytes = sizeof(Scene::VertexPos);
			vbvNormal.StrideInBytes = sizeof(Scene::VertexNormal);
			vbvTexCord.StrideInBytes = sizeof(Scene::VertexTexCord);
			vbvTangent.StrideInBytes = sizeof(Scene::VertexTangent);
			
			for (auto& prim : primitives) {
				//set mat 		
				
					UINT matvIndex = matCBVHeapIndexStart +
						curFrameIndex * model.numMaterials + prim.matIndex;
					auto matCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
					matCbvHandle.Offset(matvIndex, Graphics::gCbvSrvUavDescriptorSize);
					commandList->SetGraphicsRootDescriptorTable(3, matCbvHandle);


					//set textures 
					auto& mat = model.materials[prim.matIndex];
					UINT texDiffuseheapIndex = mat->diffuseMapSrvHeapIndex;
					auto texDiffuseCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
					texDiffuseCbvHandle.Offset(texDiffuseheapIndex, Graphics::gCbvSrvUavDescriptorSize);
					commandList->SetGraphicsRootDescriptorTable(0, texDiffuseCbvHandle);


					// set vertex/index for each render primitive
					vbvPos.SizeInBytes = prim.vertexBufferPosByteSize;
					vbvPos.BufferLocation = model.vertexPosBufferGPU->GetGPUVirtualAddress() + prim.vbPosOffset;
					commandList->IASetVertexBuffers(0, 1, &vbvPos);

					if (prim.vertexBufferTexCordByteSize>0) {
						vbvTexCord.SizeInBytes = prim.vertexBufferTexCordByteSize;
						vbvTexCord.BufferLocation = model.vertexTexCordBufferGPU->GetGPUVirtualAddress() + prim.vbTexOffset;
						commandList->IASetVertexBuffers(2, 1, &vbvTexCord);
					}

					if (prim.vbNormalBufferByteSize) {
						vbvNormal.SizeInBytes = prim.vbNormalBufferByteSize;
						vbvNormal.BufferLocation = model.vertexNormalBufferGPU->GetGPUVirtualAddress() + prim.vbNormalOffset;
						commandList->IASetVertexBuffers(1, 1, &vbvNormal);
					}
					

					commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


					if (prim.hasIndices) {
						ibv.Format = prim.iformat;
						ibv.SizeInBytes = prim.indexBufferByteSize;
						ibv.BufferLocation = model.indexBufferGPU->GetGPUVirtualAddress() + prim.ibOffset;
						commandList->IASetIndexBuffer(&ibv);
						commandList->DrawIndexedInstanced(prim.indexCount, 1, 0, 0, 0);
					}
					else {
						commandList->DrawInstanced(prim.vertexCount, 1, 0, 0);
					
					}
			
			
			}
			
		}

	}

	void RenderCubemap(ComPtr<ID3D12GraphicsCommandList> commandList) {
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = 1024;
		viewport.Height = 1024;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		commandList->RSSetViewports(1, &viewport);


		D3D12_RECT ScissorRect = { 0,0,static_cast<int>(1024), static_cast<int>(1024) };
		commandList->RSSetScissorRects(1, &ScissorRect);


		commandList->SetPipelineState(rPsoCubemap.Get());
		commandList->SetGraphicsRootSignature(rCubemapRootSignature.Get());

		//set cbvsrv descriptor heap
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);

		//clear back buffer and depth buffer
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvcubeHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvcubeHandle.Offset(Graphics::gSwapChainBufferCount, Graphics::gRTVDescriptorSize);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(1, Graphics::gDSVDescriptorSize);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rCubemapDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		D3D12_VERTEX_BUFFER_VIEW vbvPos = {};
		vbvPos.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		vbvPos.SizeInBytes = (UINT)Cubemap::vertexPosBufferByteSize;
		vbvPos.BufferLocation = Cubemap::vertexPosBufferGPU->GetGPUVirtualAddress();

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = (UINT)Cubemap::indexBufferByteSize;
		ibv.BufferLocation = Cubemap::indexBufferGPU->GetGPUVirtualAddress();

		commandList->IASetVertexBuffers(0, 1, &vbvPos);

		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&ibv);



		auto imageHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(0, imageHandle);

		DirectX::XMVECTOR lookats[6] = {
			DirectX::XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f),
			
			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f)

		};

		DirectX::XMVECTOR ups[6] = {
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f)
		};

		UINT cbvIndex = cubemappassCBVHeapIndexStart;//hdr + cubemap srv + irradiance srv
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, Graphics::gCbvSrvUavDescriptorSize);

		for (int i = 0; i < 6; i++) {
			//set cbvs
			commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
			cbvHandle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			//update pass parameters
			{

				DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				DirectX::XMVECTOR lookat = lookats[i];//DirectX::XMVectorZero();
				DirectX::XMVECTOR up = ups[i];

				//DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&gMainCam.camPos);
				//DirectX::XMVECTOR lookat = DirectX::XMVectorZero();
				//DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

				DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH(pos, lookat, up);

				DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovRH(0.5f * Math::PI, 1.0f, 0.1f, 10.0f);
				DirectX::XMMATRIX viewProj = view * proj;


				PassConstants pConsts;
				DirectX::XMStoreFloat4x4(&pConsts.ViewProjMatrix, DirectX::XMMatrixTranspose(viewProj));
				DirectX::XMStoreFloat4x4(&pConsts.ViewMatrix, DirectX::XMMatrixTranspose(view));
				DirectX::XMStoreFloat4x4(&pConsts.ProjMatrix, DirectX::XMMatrixTranspose(proj));
				pConsts.CameraPos.x = 0.0;
				pConsts.CameraPos.y = 0.0;
				pConsts.CameraPos.z = 0.0;

				pConsts.NearZ = 1.0;
				pConsts.FarZ = 10.0;
				pConsts.shaderSelector = shaderSelector;

				Cubemap::passCB[i]->CopyData(0, &pConsts);
			}



			commandList->ClearRenderTargetView(
				rtvcubeHandle,
				DirectX::Colors::Red, 0, nullptr);


			commandList->ClearDepthStencilView(
				dsvHandle,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			commandList->OMSetRenderTargets(1,
				&rtvcubeHandle,
				false,
				&dsvHandle);

			commandList->DrawIndexedInstanced(Cubemap::indexCount, 1, 0, 0, 0);


			rtvcubeHandle.Offset(1, Graphics::gRTVDescriptorSize);
			
		}
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rSkyboxArray.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}


	void RenderShadowMap(ComPtr<ID3D12GraphicsCommandList> commandList) {

		FrameResource* currentFrameResource = Graphics::gFrameResourceManager.GetCurrentFrameResource();
		if (!currentFrameResource->shadowMapRenderRequired) {
			return;
		}

		currentFrameResource->shadowMapRenderRequired = false;
		auto frameIndex = (int)Graphics::gFrameResourceManager.GetCurrentIndex();
		auto& shadowmap = Graphics::gFrameResourceManager.GetCurrentFrameResource()->shadowMap;

		commandList->RSSetViewports(1, &Graphics::gScreenViewport);
		commandList->RSSetScissorRects(1, &Graphics::gScissorRect);

		commandList->SetPipelineState(rPsoShadow.Get());
		commandList->SetGraphicsRootSignature(rRootSignature.Get());



		//todo change to shadow pass
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(shadowmapCBVHeapIndexStart+ frameIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Graphics::gFrameResourceManager.GetCurrentFrameResource()->shadowMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE ));

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(2 + frameIndex, Graphics::gDSVDescriptorSize);

		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		commandList->OMSetRenderTargets(0, nullptr, false, &dsvHandle);
		DrawRenderItems(commandList);


		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Graphics::gFrameResourceManager.GetCurrentFrameResource()->shadowMap.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	

		auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		srvHandle.Offset(shadowMapSRVHeapIndexStart+ frameIndex, Graphics::gCbvSrvUavDescriptorSize);

		//commandList->SetGraphicsRootDescriptorTable(5, srvHandle);

	}



	void RenderIrradiancemap(ComPtr<ID3D12GraphicsCommandList> commandList) {
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = 1024;
		viewport.Height = 1024;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		commandList->RSSetViewports(1, &viewport);
		D3D12_RECT ScissorRect = { 0,0,static_cast<int>(1024), static_cast<int>(1024) };
		commandList->RSSetScissorRects(1, &ScissorRect);

		commandList->SetPipelineState(rPsoIrradiance.Get());
		commandList->SetGraphicsRootSignature(rCubemapRootSignature.Get());


		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvcubeHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvcubeHandle.Offset(Graphics::gSwapChainBufferCount + 6, Graphics::gRTVDescriptorSize);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(1, Graphics::gDSVDescriptorSize);
		//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rCubemapDepthStencilBuffer.Get(),
		//	D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		D3D12_VERTEX_BUFFER_VIEW vbvPos = {};
		vbvPos.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		vbvPos.SizeInBytes = (UINT)Cubemap::vertexPosBufferByteSize;
		vbvPos.BufferLocation = Cubemap::vertexPosBufferGPU->GetGPUVirtualAddress();

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = (UINT)Cubemap::indexBufferByteSize;
		ibv.BufferLocation = Cubemap::indexBufferGPU->GetGPUVirtualAddress();

		commandList->IASetVertexBuffers(0, 1, &vbvPos);

		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&ibv);

		auto cubemapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cubemapHandle.Offset(cubemapSRVHeapIndexStart, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, cubemapHandle);

		UINT cbvIndex = cubemappassCBVHeapIndexStart;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, Graphics::gCbvSrvUavDescriptorSize);

		DirectX::XMVECTOR lookats[6] = {
			DirectX::XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f),

			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f)

		};

		DirectX::XMVECTOR ups[6] = {
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f)
		};
		

		for (int i = 0; i < 6; i++) {
			commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
			cbvHandle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			//update pass parameters
			{

				DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				DirectX::XMVECTOR lookat = lookats[i];//DirectX::XMVectorZero();
				DirectX::XMVECTOR up = ups[i];

				//DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&gMainCam.camPos);
				//DirectX::XMVECTOR lookat = DirectX::XMVectorZero();
				//DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

				DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH(pos, lookat, up);

				DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovRH(0.5f * Math::PI, 1.0f, 0.1f, 10.0f);
				DirectX::XMMATRIX viewProj = view * proj;


				PassConstants pConsts;
				DirectX::XMStoreFloat4x4(&pConsts.ViewProjMatrix, DirectX::XMMatrixTranspose(viewProj));
				DirectX::XMStoreFloat4x4(&pConsts.ViewMatrix, DirectX::XMMatrixTranspose(view));
				DirectX::XMStoreFloat4x4(&pConsts.ProjMatrix, DirectX::XMMatrixTranspose(proj));
				pConsts.CameraPos.x = 0.0;
				pConsts.CameraPos.y = 0.0;
				pConsts.CameraPos.z = 0.0;

				pConsts.NearZ = 1.0;
				pConsts.FarZ = 10.0;
				pConsts.shaderSelector = shaderSelector;

				Cubemap::passCB[i]->CopyData(0, &pConsts);
			}

			commandList->ClearRenderTargetView(
				rtvcubeHandle,
				DirectX::Colors::Red, 0, nullptr);


			commandList->ClearDepthStencilView(
				dsvHandle,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

			commandList->OMSetRenderTargets(1,
				&rtvcubeHandle,
				false,
				&dsvHandle);

			commandList->DrawIndexedInstanced(Cubemap::indexCount, 1, 0, 0, 0);

			rtvcubeHandle.Offset(1, Graphics::gRTVDescriptorSize);	
		
		}

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rIrradianceSkyboxArray.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	}

	void RenderSkyBox(ComPtr<ID3D12GraphicsCommandList> commandList) {
		commandList->RSSetViewports(1, &Graphics::gScreenViewport);
		commandList->RSSetScissorRects(1, &Graphics::gScissorRect);
		

		commandList->SetPipelineState(rPsoSkybox.Get());
		commandList->SetGraphicsRootSignature(rCubemapRootSignature.Get());
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		auto cubemapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cubemapHandle.Offset(cubemapSRVHeapIndexStart, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, cubemapHandle);

		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCBVHeapIndexStart+ curFrameIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);
		
		//set the back buffer for rendering 
		commandList->OMSetRenderTargets(1,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex,
				Graphics::gRTVDescriptorSize),
			true,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()));

		commandList->DrawInstanced(3, 1, 0, 0);
	}

	void RenderUI(ComPtr<ID3D12GraphicsCommandList> commandList) {
		commandList->OMSetRenderTargets(1, &CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
			gCurBackBufferIndex, Graphics::gRTVDescriptorSize), false, NULL
		);
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(NULL, (void*)commandList.Get());
		}
	}

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> Renderer::GetStaticSamplers()
	{


		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC shadow(
			6, // shaderRegister
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
			0.0f,                               // mipLODBias
			16,                                 // maxAnisotropy
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

		return {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp, shadow };
	}
	

	
}