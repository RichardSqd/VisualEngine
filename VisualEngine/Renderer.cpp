#include "pch.h"
#include "Renderer.h"

#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace Renderer {
	
	ComPtr<ID3D12RootSignature> rRootSignature = nullptr;
	ComPtr<ID3DBlob> rVertexShader = nullptr;
	ComPtr<ID3DBlob> rPixelShader = nullptr;
	ComPtr<ID3D12PipelineState> rPso = nullptr;
	ComPtr<ID3D12PipelineState> rPsoShadow = nullptr;
	D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc = {};
	CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc = {};
	

	void Init() {
		//ASSERT(Scene::LoadScene(Config::gltfFilePath, EngineCore::eModel));
		ASSERT(Scene::LoadTestScene(Config::testSceneFilePath, EngineCore::eModel));
		CreateShadersAndInputLayout();
		CreateRootSigniture();
		CreatePipelineState();
	}



	void CreateRootSigniture() {
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		BREAKIFFAILED(Graphics::gDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)));

		CD3DX12_DESCRIPTOR_RANGE1 ranges[4] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //diffuse and normal textures, register t1,2 
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //1 constant buffer 
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //shadow texture
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[4] = {};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> serializedRootSig;
		ComPtr<ID3DBlob> err;
		BREAKIFFAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &serializedRootSig, &err));
		
		BREAKIFFAILED(Graphics::gDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(rRootSignature.GetAddressOf())));
	}

	

	void CreateShadersAndInputLayout() {
#if defined(DEBUG) || defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
		ComPtr<ID3DBlob> errors;
		BREAKIFFAILED(D3DCompileFromFile(Config::shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &rVertexShader, &errors));
		//Utils::Print((char*)errors->GetBufferPointer());
		BREAKIFFAILED(D3DCompileFromFile(Config::shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &rPixelShader, &errors));


		rInputLayoutDesc.pInputElementDescs = Scene::inputLayoutDesc;
		rInputLayoutDesc.NumElements = _countof(Scene::inputLayoutDesc);

	}

	
	void CreatePipelineState() {


		rDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		int n = (int)rVertexShader->GetBufferSize();
		Utils::Print("\nVertex Shader size is:");
		Utils::Print(std::to_string(n).c_str());
		Utils::Print("\n");
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
		psoDesc.RTVFormats[0] = Config::BackBufferFormat;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1; //no msaa for now 
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPso)));

		//shadow map pso
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0,0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoShadow)));


	}

	void CreateFrameResources() {

	}
	

	
}