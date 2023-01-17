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
	float gWidth = 1280.0;
	float gHeight = 720.0;
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
	UINT gObjectCBByteSize;
	UINT gPassCBByteSize;
	UINT gNumFrameResources = Config::frameCount;
	UINT gFrameIndex = 0;
	DXGI_FORMAT gBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT gDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	D3D12_VIEWPORT gScreenViewport;

	D3D12_RECT gScissorRect;

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

		//display port stats
		gScreenViewport.TopLeftX = 0.0f;
		gScreenViewport.TopLeftY = 0.0f;
		gScreenViewport.Width = gWidth;
		gScreenViewport.Height = gHeight;
		gScreenViewport.MinDepth = 0.0f;
		gScreenViewport.MaxDepth = 1.0f;
		
		gScissorRect = { 0, 0, static_cast<long>(gWidth), static_cast<long>(gHeight) };

		//get descriptor sizes for each type 
		gRTVDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		gDSVDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		gCbvSrvUavDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gSamplerDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);


		gCommandQueueManager.CreateCommandQueueObjects(gDevice);
		gCommandContextManager.CreateCommandContext(gDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		
		gObjectCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
		gPassCBByteSize = (sizeof(PassConstants) + 255) & ~255;

	}


	
#if defined(DEBUG) || defined(_DEBUG)

#endif

	//create command related components

	float AspectRatio()
	{
		return gWidth / gHeight;
	}
}



