#include "pch.h"
#include "Graphics.h"
#include "CommandListManager.h"

#pragma comment(lib, "d3d12.lib") 
#pragma comment(lib, "dxgi.lib") 
using Microsoft::WRL::ComPtr;


namespace Graphics {
	uint32_t gWidth = 1920;
	uint32_t gHeight = 1080;
	HWND ghWnd = nullptr;
	ComPtr<ID3D12Device> gDevice = nullptr;
	CommandListManager gCommandListManager;
	D3D_FEATURE_LEVEL gD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	

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
		ComPtr<IDXGIFactory6> dxgiFactory;
		BREAKIFFAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		//create graphic adapter
		ComPtr<IDXGIAdapter1> adapter;
		//DXGI_ADAPTER_DESC1 adapterDesc;

		//loop over each adapter available
		ULONG_PTR maxSize = 0;
		for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(i, &adapter); i++) {
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

		gCommandListManager.CreateCommandObjects(gDevice);





	}


	
#if defined(DEBUG) || defined(_DEBUG)
	//TODO: Infoqueue for profiling 
#endif

	//create command related components
	
	//D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	//queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
}