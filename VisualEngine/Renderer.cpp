#include "pch.h"
#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include "Control.h"


#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "D3D12.lib")
//#pragma comment(lib, "dxgi.lib") 
//#pragma comment(lib, "dxguid.lib")

namespace Renderer {
	
	ComPtr<ID3D12RootSignature> rRootSignature = nullptr;
	ComPtr<ID3DBlob> rVertexShader = nullptr;
	ComPtr<ID3DBlob> rPixelShader = nullptr;
	ComPtr<ID3D12PipelineState> rPso = nullptr;
	ComPtr<ID3D12PipelineState> rPsoShadow = nullptr;
	ComPtr<ID3D12GraphicsCommandList> rCommandList = nullptr;
	ComPtr<ID3D12CommandAllocator> rCommandAlloc = nullptr;
	D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc {};
	CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc {};
	
	ComPtr<ID3D12Resource> rRenderTargetBuffer[Config::frameCount];
	ComPtr<ID3D12Resource> rDepthStencilBuffer;

	D3D12_VIEWPORT gScreenViewport {};
	INT gCurBackBufferIndex = 0;

	DirectX::XMFLOAT4X4 gview{};

	Camera gMainCam{};


	void Init(CommandContext* context) {
		rCommandList = context->getCommandList();
		rCommandAlloc = context->getCommandAllocator();
		std::wstring filepath = Config::gltfFileDirectory + L"\\" + Config::gltfFileName;
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

		CD3DX12_DESCRIPTOR_RANGE1 ranges[5] = {};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //diffuse, normal  and metalicroughness textures, register t1,2,3 
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //object constant buffer
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //global constant buffer
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); //mat constant buffer
		ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //shadow texture
		//ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[5] = {};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);
		//rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = GetStaticSamplers();

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		BREAKIFFAILED(D3DCompileFromFile(Config::shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &rVertexShader, &errors));
		//Utils::Print((char*)errors->GetBufferPointer());
		BREAKIFFAILED(D3DCompileFromFile(Config::shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags, 0, &rPixelShader, &errors));


		rInputLayoutDesc.pInputElementDescs = Scene::inputLayoutDesc;
		rInputLayoutDesc.NumElements = _countof(Scene::inputLayoutDesc);

	}

	
	void CreatePipelineState() {


		rDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		int n = (int)rVertexShader->GetBufferSize();
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
		if (Config::WIREFRAME_MODE) {
			psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		}
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

		//shadow map pso
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0,0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&rPsoShadow)));


	}

	void CreateFrameResources() {
		Graphics::gFrameResourceManager.CreateFrameResources(Graphics::gNumFrameResources);
	}

	

	void CreateConstantBufferViews() {
		UINT objectCBByteSize = Graphics::gObjectCBByteSize;
		UINT objCount = EngineCore::eModel.numPrimitives;


		//generate constant buffer views for all frame resources 
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto objCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->objCB->GetResource();

			for (UINT objIndex = 0; objIndex < objCount; objIndex++) {
				
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objCB->GetGPUVirtualAddress();

				//offset to each object constant buffer
				cbAddress += static_cast<unsigned long long>(objIndex) * objectCBByteSize;

				//offset to the object cbv in the descriptor heap 
				int heapIndex = frameIndex * objCount + objIndex;
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
			
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = objectCBByteSize;
			
				Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			}

		}

		UINT passCBByteSize = Graphics::gPassCBByteSize;
		//pass CBVs 
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto passCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->passCB->GetResource();

			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

			//offset to each pass constant buffer
			int heapIndex = frameIndex + objCount * Graphics::gNumFrameResources;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);


			//Utils::Print(std::to_string(frameIndex).c_str());
			//Utils::Print(std::to_string(heapIndex).c_str());

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = passCBByteSize;
			Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
		}

		 
		UINT matCBByteSize = Graphics::gMatCBByteSize;
		UINT matCount = EngineCore::eModel.materials.size();

		//create material views
		for (UINT frameIndex = 0; frameIndex < Graphics::gNumFrameResources; frameIndex++) {
			auto matCB = Graphics::gFrameResourceManager.GetFrameResourceByIndex(frameIndex)->matCB->GetResource();
			for (UINT matIndex = 0; matIndex < matCount; matIndex++) {
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = matCB->GetGPUVirtualAddress();
				//offset to each mat const buffer 
				cbAddress += static_cast<unsigned long long>(matIndex) * matCBByteSize;

				//offset to the object cbv in the descriptor heap 
				int heapIndex = (objCount + 1) * Graphics::gNumFrameResources + 
								frameIndex * matCount + matIndex;
				//Utils::Print(std::to_wstring(heapIndex).c_str());
				
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = matCBByteSize;
				Graphics::gDevice->CreateConstantBufferView(&cbvDesc, handle);
			}

		}


	}


	void CreateShaderResourceViews() {
		//create texture heap descriptors and associate them with loaded resources
		UINT objCount = EngineCore::eModel.numPrimitives;
		UINT matCount = EngineCore::eModel.materials.size();
		UINT textureCount = EngineCore::eModel.textures.size();
		auto& textures = EngineCore::eModel.textures;

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(Graphics::gCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		//1xpass + objCount for each frame resource and matCount for each materials 
		int heapIndex = (1 + objCount + matCount) * Graphics::gNumFrameResources ;

		for (UINT i = 0; i < EngineCore::eModel.materials.size(); i++) {
			auto& mat = EngineCore::eModel.materials[i];
			if (!mat->texDiffuseMap.empty()) {
				mat->diffuseMapSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texDiffuseMap]->textureResource;
				auto des = texResource->GetDesc();
				srvDesc.Format = des.Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
				heapIndex++;
			}

			if (!mat->texroughnessMetallicMap.empty()) {
				mat->roughnessMetallicMapSrvHeaIndex = heapIndex;
				auto& texResource = textures[mat->texroughnessMetallicMap]->textureResource;
				srvDesc.Format = texResource->GetDesc().Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
				heapIndex++;
			}

			if (!mat->texNormalMap.empty()) {
				mat->normalMapSrvHeapIndex = heapIndex;
				auto& texResource = textures[mat->texNormalMap]->textureResource;
				srvDesc.Format = texResource->GetDesc().Format;
				srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
				handle.Offset(heapIndex, Graphics::gCbvSrvUavDescriptorSize);
				Graphics::gDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
				
				heapIndex++;
			}

		}
	}

	void CreateDescriptorHeaps() {
		//create rtv desc heap 
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.NumDescriptors =Graphics::gSwapChainBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(Graphics::gRtvHeap.GetAddressOf())));

		//create dsv desc heap (one dsv for each frame buffers and one for the scene)
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors =  1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		BREAKIFFAILED(Graphics::gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(Graphics::gDsvHeap.GetAddressOf())));

		//create srv/cbv desc heap 
		UINT numDescriptors = (static_cast<unsigned long long>(EngineCore::eModel.numPrimitives) 
			+ 1 + EngineCore::eModel.materials.size()) * Graphics::gNumFrameResources + EngineCore::eModel.textures.size();
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
			//Graphics::gSwapChain->GetBuffer
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
		
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * Math::PI, Graphics::AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&gMainCam.proj, P);


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

	}

	void UpdateInput() {
		/*
		auto kb = Control::m_keyboard->GetState();
		auto mouse = Control::m_mouse->GetState();
		
		
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(mouse.x - Control::lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(mouse.y - Control::lastMousePos.y));

		gMainCam.camTheta += dx;
		gMainCam.camPhi += dy;

		
		gMainCam.camPhi = Math::Clamp(gMainCam.camPhi, 0.1f, Math::PI - 0.1f);
		
		Control::lastMousePos.x = mouse.x;
		Control::lastMousePos.x = mouse.y;
		//update camera data based on mouse input 
		std::wstring mouseData = L"\n x: "+ std::to_wstring(mouse.x) +L" y:"+ std::to_wstring(mouse.y) + L"\n";
		Utils::Print(mouseData.c_str() );

		*/
	}

	void UpdateCamera() {
		gMainCam.camPos.x = gMainCam.camRadius * sinf(gMainCam.camPhi) * cosf(gMainCam.camTheta);
		gMainCam.camPos.y = gMainCam.camRadius * cosf(gMainCam.camPhi);
		gMainCam.camPos.z = gMainCam.camRadius * sinf(gMainCam.camPhi) * sinf(gMainCam.camTheta);

		//std::wstring text = L"\n "+ std::to_wstring(gMainCam.camPos.x) + L" " + std::to_wstring(gMainCam.camPos.y) + L" " + std::to_wstring(gMainCam.camPos.z)+L"\n ";
		//Utils::Print(text.c_str());
		//view matrix update
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(gMainCam.camPos.x, gMainCam.camPos.y, gMainCam.camPos.z, 1.0f);
		DirectX::XMVECTOR lookat = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, lookat, up);
		DirectX::XMStoreFloat4x4(&gMainCam.view, view);
	}
	
	void UpdateObjCBs(FrameResource* currentFrameResource) {
		auto curFrameObjCB = currentFrameResource->objCB.get();
		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
			auto& node = EngineCore::eModel.nodes[i];
			if (node.numFrameDirty > 0) {
				Utils::Print("Update OBJ CB LOOP");
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&node.matrix);

				ObjectConstants objConsts;
				DirectX::XMStoreFloat4x4(&objConsts.World, world);
				
				DirectX::XMStoreFloat4x4(&objConsts.WorldIT, Math::InverseTranspose(world));
				
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
				materialConsts.roughnessFactor = mat->roughness;
				curFrameMatCB->CopyData(i, &materialConsts);
				mat->numFrameDirty--;
			}
		}


	}


	void UpdatePassCB(FrameResource* currentFrameResource){
		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&gMainCam.view);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&gMainCam.proj);
		DirectX::XMMATRIX viewProj = view * proj;

		//todo:
		PassConstants pConsts;
		DirectX::XMStoreFloat4x4(&pConsts.ViewProjMatrix, XMMatrixTranspose(viewProj));
		pConsts.CameraPos = gMainCam.camPos;
		pConsts.NearZ = 1.0;
		pConsts.FarZ = 1000.0;


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
		BREAKIFFAILED(Graphics::gSwapChain->Present(1, 0));
		gCurBackBufferIndex = (gCurBackBufferIndex + 1) % Graphics::gSwapChainBufferCount;

		//fence move to new position and signal the new GPU fence value 
		queue.AdvanceFenceValue();
		Graphics::gFrameResourceManager.GetCurrentFrameResource()->fence = queue.GetCurrentFenceValue();
		//std::wstring text = L"frame fence" + std::to_wstring(queue.GetCurrentFenceValue()) + L"  " +L"\n";
		//Utils::Print(text.c_str());
		queue.SignalFencePoint();

	}

	void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> commandList) {
		//Reset and reuse command allocator
		//BREAKIFFAILED(Renderer::rCommandAlloc->Reset());

		//Reset Command list and reuse the memory
		//BREAKIFFAILED(rCommandList->Reset(Renderer::rCommandAlloc.Get(), rPso.Get()));

		commandList->RSSetViewports(1, &Graphics::gScreenViewport);
		commandList->RSSetScissorRects(1, &Graphics::gScissorRect);

		//back buffer transition state from present to render target 
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rRenderTargetBuffer[gCurBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		//clear back buffer and depth buffer
		commandList->ClearRenderTargetView(
			CD3DX12_CPU_DESCRIPTOR_HANDLE( Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex, Graphics::gRTVDescriptorSize),
			DirectX::Colors::LightGray, 0, nullptr);
		commandList->ClearDepthStencilView(
			CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()),
			 D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


		//set the back buffer for rendering 
		commandList->OMSetRenderTargets(1,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gRtvHeap->GetCPUDescriptorHandleForHeapStart(),
				gCurBackBufferIndex, Graphics::gRTVDescriptorSize),
			true,
			&CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gDsvHeap->GetCPUDescriptorHandleForHeapStart()));



		//set cbvsrv descriptor heap
		ID3D12DescriptorHeap* descriptorHeaps[] = { Graphics::gCbvSrvHeap.Get()  };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);
		commandList->SetGraphicsRootSignature(rRootSignature.Get());

		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
		
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		int passCbvIndex = curFrameIndex + EngineCore::eModel.numNodes * Graphics::gNumFrameResources;
		passCbvHandle.Offset(passCbvIndex, Graphics::gCbvSrvUavDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);


		DrawRenderItems(commandList);

		//back buffer transition state from render target back to present
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rRenderTargetBuffer[gCurBackBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		//finish populating commandlist
		BREAKIFFAILED(commandList->Close());

	}

	
	
	void DrawRenderItems(ComPtr<ID3D12GraphicsCommandList> commandList) {

		Scene::Model& model = EngineCore::eModel;
		UINT objCount = model.numNodes;
		UINT matCount = model.numMaterials;
		
		UINT curFrameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {

			auto& node = model.nodes[i];
			auto& primitives = model.meshes[node.mesh].primitives;


			// set obj CBV in the descritpor heap for each node for the current frame resource
			//todo: object constant buffer view index can be saved as a member of nodes
			UINT cbvIndex = curFrameIndex * EngineCore::eModel.numNodes + i;
			auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
			cbvHandle.Offset(cbvIndex, Graphics::gCbvSrvUavDescriptorSize);
			commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);


			D3D12_VERTEX_BUFFER_VIEW vbvPos = {}; 
			D3D12_VERTEX_BUFFER_VIEW vbvTex = {};
			D3D12_INDEX_BUFFER_VIEW ivb = {};

			ivb.BufferLocation = model.indexBufferGPU->GetGPUVirtualAddress();

			vbvPos.BufferLocation = model.vertexPosBufferGPU->GetGPUVirtualAddress();
			vbvPos.StrideInBytes = sizeof(Scene::VertexPos);

			vbvTex.BufferLocation = model.vertexTexCordBufferGPU->GetGPUVirtualAddress();
			vbvTex.StrideInBytes = sizeof(Scene::VertexTexCord);

			

			for (auto& prim : primitives) {
				//set mat 
				//curMat = model.materials[ prim.matName]->
				UINT matvIndex = (model.numNodes + 1) * Graphics::gNumFrameResources +
					curFrameIndex * model.numMaterials + prim.matIndex;  // Graphics::gFrameResourceManager.GetCurrentIndex() * EngineCore::eModel.numNodes + i;
				auto matCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
				matCbvHandle.Offset(matvIndex, Graphics::gCbvSrvUavDescriptorSize);
				commandList->SetGraphicsRootDescriptorTable(3, matCbvHandle);


				//set texture buffer  
				int texheapIndex = (1 + objCount + matCount) * Graphics::gNumFrameResources;
				auto texCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
				texCbvHandle.Offset(texheapIndex, Graphics::gCbvSrvUavDescriptorSize);
				commandList->SetGraphicsRootDescriptorTable(0, texCbvHandle);

				// set vertex/index for each render object
				ivb.Format = DXGI_FORMAT_R16_UINT;
				ivb.SizeInBytes = prim.indexBufferByteSize;
	
				vbvPos.SizeInBytes = prim.vertexBufferPosByteSize;
				commandList->IASetVertexBuffers(0, 1, &vbvPos);

				vbvTex.SizeInBytes = prim.vertexBufferTexCordByteSize;
				commandList->IASetVertexBuffers(2, 1, &vbvTex);
					
				commandList->IASetIndexBuffer(&ivb);
				commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->DrawIndexedInstanced(prim.indexCount, 1, prim.ibOffset, prim.vbPosOffset, 0);
			
			}
			
			//commandList->IASetVertexBuffers(4, 1, &model.vertexColorBufferView);	

			//set mat cbv in the descritpor heap for each node for the current frame resource
			
			//commandList->SetGraphicsRootDescriptorTable(3, matCbvHandle);
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