#include "pch.h"
#include "Graphics.h"
#include "Model.h"
#include "CommandListManager.h"
#include "EngineCore.h"

#pragma comment(lib, "d3d12.lib") 
#pragma comment(lib, "dxgi.lib") 
#pragma comment(lib, "dxguid.lib")
using Microsoft::WRL::ComPtr;


namespace Graphics {
	uint32_t gWidth = 1280;
	uint32_t gHeight = 720;
	HWND ghWnd = nullptr;
	ComPtr<ID3D12Device> gDevice = nullptr;
	ComPtr<IDXGISwapChain3> gSwapChain = nullptr;
	ComPtr<ID3D12DescriptorHeap> gRtvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> gDsvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> gCbvSrvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> gSamplerHeap = nullptr;
	CommandQueueManager gCommandQueueManager {};
	CommandContextManager gCommandContextManager{};
	FrameResourceManager gFrameResourceManager {};

	D3D_FEATURE_LEVEL gD3DFeatureLevel = D3D_FEATURE_LEVEL_12_2;
	ComPtr<IDXGIFactory6> gdxgiFactory;
	UINT gRTVDescriptorSize;
	UINT gDSVDescriptorSize;
	UINT gCbvSrvUavDescriptorSize;
	UINT gSamplerDescriptorSize;
	UINT gNumFrameResources = 3;
	UINT gNumFrameBuffers = 3;
	UINT gFrameIndex = 0;

	void Init(bool EnableDXR) {
		ComPtr<ID3D12Device> device;
		uint32_t dxgiFactoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		{
			ComPtr<ID3D12Debug> debugController;
			ComPtr<ID3D12Debug1> debugController1;
			BREAKIFFAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
			if (Config::useGPUBasedValidation) {
				if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
					debugController1->SetEnableGPUBasedValidation(true);
				}
			}
		}
#endif

		//For creating adapters and swap chain
		
		BREAKIFFAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&gdxgiFactory)));

		//create graphic adapter
		ComPtr<IDXGIAdapter1> adapter;
		//DXGI_ADAPTER_DESC1 adapterDesc;

		//loop over each adapter available
		ULONG_PTR maxSize = 0;
		for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != gdxgiFactory->EnumAdapters1(i, &adapter); i++) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (FAILED(D3D12CreateDevice(adapter.Get(), gD3DFeatureLevel, IID_PPV_ARGS(&device))))
				continue;
			if (desc.DedicatedVideoMemory < maxSize)
				continue;
			maxSize = desc.DedicatedVideoMemory;
			if (gDevice != nullptr) gDevice->Release();
			gDevice = device.Detach();
			Utils::Print(desc.Description);
		}
		BREAKIFNULL(gDevice);

#if defined(DEBUG) || defined(_DEBUG)
		gDevice->SetStablePowerState(1);
#endif
		//get descriptor sizes for each type 
		gRTVDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		gDSVDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		gCbvSrvUavDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gSamplerDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);


		gCommandQueueManager.CreateCommandObjects(gDevice);
		gCommandContextManager.CreateCommandContext(gDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		CreateSwapChain();
		
		CreateDescriptorHeaps();
		

	}


	
#if defined(DEBUG) || defined(_DEBUG)

#endif

	//create command related components
}

void Graphics::CreateSwapChain() {
	ComPtr<IDXGISwapChain1> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = gWidth; 
	swapChainDesc.Height = gHeight;
	swapChainDesc.Format = Config::BackBufferFormat;
	swapChainDesc.BufferCount = gNumFrameBuffers;
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

	BREAKIFFAILED(gdxgiFactory->CreateSwapChainForHwnd(
		gCommandQueueManager.GetGraphicsQueue().GetQueue().Get(),
		ghWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	BREAKIFFAILED(swapChain.As(&gSwapChain));
	BREAKIFFAILED(gdxgiFactory->MakeWindowAssociation(ghWnd, DXGI_MWA_NO_ALT_ENTER));
	gFrameIndex = gSwapChain->GetCurrentBackBufferIndex();

}

void Graphics::CreateDescriptorHeaps() {
	//create rtv desc heap 
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = gNumFrameBuffers;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	BREAKIFFAILED(gDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(gRtvHeap.GetAddressOf())));

	//create dsv desc heap (one dsv for each frame buffers and one for the scene)
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = gNumFrameBuffers + 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	BREAKIFFAILED(gDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(gDsvHeap.GetAddressOf())));

	//create srv/cbv desc heap 
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc;
	cbvSrvHeapDesc.NumDescriptors = 4096;
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvSrvHeapDesc.NodeMask = 0;
	BREAKIFFAILED(gDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(gCbvSrvHeap.GetAddressOf())));

	//create sampler desc heap 
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc;
	samplerHeapDesc.NumDescriptors = 2048;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	samplerHeapDesc.NodeMask = 0;
	BREAKIFFAILED(gDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(gSamplerHeap.GetAddressOf())));

}

