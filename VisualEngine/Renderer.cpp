#include "pch.h"
#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include "Control.h"
#include "imgui_impl_dx12.h"
#include "ShaderLightingData.h"
#include "Cubemap.h"

#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "D3D12.lib")
//#pragma comment(lib, "dxgi.lib") 
//#pragma comment(lib, "dxguid.lib")

namespace Renderer {
	
	ComPtr<ID3D12RootSignature> rRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> rCubemapRootSignature = nullptr;
	ComPtr<ID3DBlob> rVertexShader = nullptr;
	ComPtr<ID3DBlob> rPixelShader = nullptr;
	ComPtr<ID3DBlob> rPixelPBRShader = nullptr;
	ComPtr<ID3DBlob> rCubemapVertexShader = nullptr;
	ComPtr<ID3DBlob> rCubemapPixelShader = nullptr;

	ComPtr<ID3D12PipelineState> rPso = nullptr;
	ComPtr<ID3D12PipelineState> rPsoShadow = nullptr;
	ComPtr<ID3D12PipelineState> rPsoWireframe = nullptr;
	ComPtr<ID3D12PipelineState> rPsoPBR = nullptr;
	ComPtr<ID3D12PipelineState> rPsoCubemap = nullptr;
	ComPtr<ID3D12GraphicsCommandList> rCommandList = nullptr;
	ComPtr<ID3D12CommandAllocator> rCommandAlloc = nullptr;
	D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc {};
	D3D12_INPUT_LAYOUT_DESC rCubemapInputLayoutDesc{};
	CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc {};
	
	ComPtr<ID3D12Resource> rRenderTargetBuffer[Config::numRenderTargets];
	ComPtr<ID3D12Resource> rDepthStencilBuffer;
	ComPtr<ID3D12Resource> rCubemapDepthStencilBuffer;
	ComPtr<ID3D12Resource> rSkybox[6];
	
	D3D12_VIEWPORT gScreenViewport {};
	INT gCurBackBufferIndex = 0;

	DirectX::XMFLOAT4X4 gview{};

	Camera gMainCam{};

	//UI interactable parameters
	int rframeRateCap60 = 1;
	int shaderSelector = 0;
	bool wireframeMode = 0;

	UINT objectCBVHeapIndexStart = 0;
	UINT passCBVHeapIndexStart = 0;
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
		CreateConstantBufferViews();
		CreateShaderResourceViews();
		CreateCubemapResources();
		
	}

	void InitCamera() {
		gMainCam.camPhi =  DirectX::XM_PIDIV4;
		gMainCam.camTheta = 1.5f * DirectX::XM_PI;
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

		CD3DX12_DESCRIPTOR_RANGE1 ranges[6] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //diffuse,metalicroughness, normal, AO textures, register t1,2,3 
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //object constant buffer
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //global constant buffer
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //mat constant buffer
		ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //light constant buffer
		ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //shadow texture
		//ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[6] = {};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[5].InitAsDescriptorTable(1, &ranges[5], D3D12_SHADER_VISIBILITY_PIXEL);
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
	
	
		//cubemap 
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


		D3DCompileFromFile(Config::cubemapPixelShaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rCubemapPixelShader, &errors);
		if (errors != nullptr && errors->GetBufferSize() > 0) {
			Utils::Print((char*)errors->GetBufferPointer());
		}
		

		rInputLayoutDesc.pInputElementDescs = Scene::inputLayoutDesc;
		rInputLayoutDesc.NumElements = _countof(Scene::inputLayoutDesc);

		rCubemapInputLayoutDesc.pInputElementDescs = Scene::cubeMapinputLayoutDesc;
		rCubemapInputLayoutDesc.NumElements = _countof(Scene::cubeMapinputLayoutDesc);
	}

	
	void CreatePipelineState() {


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
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoWireframe)));

		//pbr
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(rPixelPBRShader.Get());
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoPBR)));


		//create cubemap pso
		psoDesc.InputLayout = rCubemapInputLayoutDesc;
		psoDesc.pRootSignature = rCubemapRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(rCubemapVertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(rCubemapPixelShader.Get());

		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoCubemap)));


		//shadow map pso
		//psoDesc.PS = CD3DX12_SHADER_BYTECODE(0,0);
		//psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		//psoDesc.NumRenderTargets = 0;
		//BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoShadow)));		

	}

	void UpdatePipelineState() {

	}

	void CreateFrameResources() {
		Graphics::gFrameResourceManager.CreateFrameResources(Graphics::gNumFrameResources);
		for (int i = 0; i < 6; i++) {
			Cubemap::passCB.push_back(std::make_unique<UploadBuffer>(sizeof(PassConstants), 1, true));
		}
	}

	

	void CreateConstantBufferViews() {
		auto& model = EngineCore::eModel;
		UINT objectCBByteSize = Graphics::gObjectCBByteSize;
		UINT primCount = model.numPrimitives;


		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			int heapIndex = 0;
			auto& texResource = model.textures["IBL_Texture"]->textureResource;
			srvDesc.Format = texResource->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
			Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);

			
		}


		objectCBVHeapIndexStart = 1+6+6;

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
		
		lightCBVHeapIndexStart = passCBVHeapIndexStart + Graphics::gNumFrameResources;
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
		rtvHeapDesc.NumDescriptors = Graphics::gSwapChainBufferCount + 6;//hdr cube map render target
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(Graphics::gRtvHeap.GetAddressOf())));

		//create dsv desc heap (one dsv for each frame buffers and one for the scene)
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors =  1 + 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(Graphics::gDsvHeap.GetAddressOf())));

		//create srv/cbv desc heap 
		UINT numDescriptors = (static_cast<unsigned long long>(EngineCore::eModel.numNodes) 
			+ 1 //skybox
			+ 6 //cubemaps as texture resources
			+ 6 //cubemap global parameters 
			+ (UINT)EngineCore::eModel.materials.size()) * Graphics::gNumFrameResources //materials 
			+ static_cast<unsigned long long>((UINT)EngineCore::eModel.materials.size()) * 5 //material textures
			+ 1 * static_cast<unsigned long long>(Graphics::gNumFrameResources) //pass cbv
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
		DirectX::XMMATRIX P2 = DirectX::XMMatrixPerspectiveFovRH(0.25f * Math::PI, Graphics::AspectRatio(), 0.1f, 1000.0f);

		
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
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(Graphics::gSwapChainBufferCount, Graphics::gRTVDescriptorSize);


		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = 1.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;


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


		for (int i = 0; i < 6; i++) {

			Graphics::gDevice.Get()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COMMON,
				&clearValue,
				IID_PPV_ARGS(rSkybox[i].ReleaseAndGetAddressOf()));


			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.Texture2D.MipLevels = 1;

			//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			//uavDesc.Format = textureDesc.Format;
			//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			//uavDesc.Texture2D.MipSlice = 0;

			auto& texResource = rSkybox[i];
			
			
			Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
			handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
			

			D3D12_RENDER_TARGET_VIEW_DESC cubemapImagesRTV = {};
			cubemapImagesRTV.Format = textureDesc.Format;
			cubemapImagesRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			cubemapImagesRTV.Texture2D.MipSlice = 0;
			
			
			Graphics::gDevice->CreateRenderTargetView(texResource.Get(), &cubemapImagesRTV, rtvHandle);
			rtvHandle.Offset(1, Graphics::gRTVDescriptorSize);

		}

		//pass constant buffer view
		for (int i = 0; i < 6; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = Cubemap::passCB[i]->GetResource()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = Graphics::gPassCBByteSize;
			Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			handle.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
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
		UpdateAnimation();

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


		UpdateObjCBs(currentFrameResource);
		UpdateMaterialCBs(currentFrameResource);
		UpdatePassCB(currentFrameResource);
		UpdateLightCBs(currentFrameResource);
	}

	void UpdateAnimation() {

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

				DirectX::XMFLOAT4 sceneScaling = Scene::sceneScaling;
				DirectX::XMFLOAT4 sceneTranslation = Scene::sceneTranslation;
				DirectX::XMMATRIX temp = 
					DirectX::XMMatrixScaling(sceneScaling.x, sceneScaling.y, sceneScaling.z) 
					* DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat4(&Scene::axis), Scene::angle) 
					* DirectX::XMMatrixTranslation(sceneTranslation.x, sceneTranslation.y, sceneTranslation.z);
				world = world * temp ;
				
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
			LightConstant lc = {};
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
		pConsts.FarZ = 1000.0;
		pConsts.shaderSelector = shaderSelector;

		auto curFramePassCB = currentFrameResource->passCB.get();
		curFramePassCB->CopyData(0, &pConsts);


	}


	void Draw() {
		
		auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
		auto& commandContext = Graphics::gFrameResourceManager.GetCurrentFrameResource()->comandContext;
		auto allocator = commandContext->getCommandAllocator();
		auto commandList = commandContext->getCommandList();

		//Reuse the command allocator for the current frame, as we know 
		//the last commands in queue has been completed for this frame 
		
		BREAKIFFAILED(allocator->Reset());

		

		BREAKIFFAILED(commandList->Reset(allocator.Get(), rPso.Get()));
		

		PopulateCommandList(commandList);
		
		//add the command list to queue
		ID3D12CommandList* cmdsLists[] = { commandList.Get() };
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

		//set cbvsrv descriptor heap
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get()  };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);


		//prepare for the skybox from hdr image
		if (runAtIFirstIteration) {
			renderCubemap(commandList);
			runAtIFirstIteration = false;
		}

		commandList->RSSetViewports(1, &Graphics::gScreenViewport);
		commandList->RSSetScissorRects(1, &Graphics::gScissorRect);
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


		//set the back buffer for rendering 
		commandList->OMSetRenderTargets(1,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex,
				Graphics::gRTVDescriptorSize),
			true,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()));

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

		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
		
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		int passCbvIndex = passCBVHeapIndexStart;
		passCbvHandle.Offset(passCbvIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);

		//lights handle
		//UINT lightCount = (UINT)EngineCore::eModel.lights.size();
		
		auto lightCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		int lightCbvIndex = lightCBVHeapIndexStart + curFrameIndex;
		lightCbvHandle.Offset(lightCbvIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(4, lightCbvHandle);

		DrawRenderItems(commandList);

		//RenderSkyBox(commandList);
		RenderUI(commandList);
		

		//back buffer transition state from render target back to present
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rRenderTargetBuffer[gCurBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		//finish populating commandlist
		BREAKIFFAILED(commandList->Close());

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

	void renderCubemap(ComPtr<ID3D12GraphicsCommandList> commandList) {
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
		auto& commandContext = Graphics::gFrameResourceManager.GetCurrentFrameResource()->comandContext;


		commandList->SetPipelineState(rPsoCubemap.Get());
		commandList->SetGraphicsRootSignature(rCubemapRootSignature.Get());


		//clear back buffer and depth buffer
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvcubeHandle(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvcubeHandle.Offset(Graphics::gSwapChainBufferCount, Graphics::gRTVDescriptorSize);



		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsvHandle.Offset(1, Graphics::gDSVDescriptorSize);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rCubemapDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		D3D12_VERTEX_BUFFER_VIEW vbvPos = {};
		vbvPos.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		vbvPos.SizeInBytes = Cubemap::vertexPosBufferByteSize;
		vbvPos.BufferLocation = Cubemap::vertexPosBufferGPU->GetGPUVirtualAddress();

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = Cubemap::indexBufferByteSize;
		ibv.BufferLocation = Cubemap::indexBufferGPU->GetGPUVirtualAddress();

		commandList->IASetVertexBuffers(0, 1, &vbvPos);

		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&ibv);



		auto imageHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(0, imageHandle);

		DirectX::XMVECTOR lookats[6] = {
			DirectX::XMVectorSet(1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f)

		};

		DirectX::XMVECTOR ups[6] = {
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f,  1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f,  0.0f, -1.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, -1.0f,  0.0f, 0.0f)
		};

		UINT cbvIndex = 7;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, Graphics::gCbvSrvUavDescriptorSize);

		for (int i = 0; i < 6; i++) {
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rSkybox[i].Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));



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
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rSkybox[i].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	void RenderSkyBox(ComPtr<ID3D12GraphicsCommandList> commandList) {
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);
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

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Renderer::GetStaticSamplers()
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

		return {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp };
	}
	

	
}