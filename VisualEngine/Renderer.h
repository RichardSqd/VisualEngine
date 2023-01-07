#pragma once
#include "SceneLoader.h"
#include "Graphics.h"
#include "EngineCore.h"

namespace Renderer {
	extern void Init();
	extern void CreateRootSigniture();
	extern void CreatePipelineState();
	extern void CreateShadersAndInputLayout();
	extern void CreateRTVDSV();
	extern void CreateDescriptorHeaps();
	extern void CreateFrameResources();
	extern void CreateSwapChain();
	extern void CreateConstantBufferViews();
	extern void Update();
	extern void UpdateCamera();
	extern void UpdateObjCBs();
	extern void UpdatePassCB();
	extern void Draw();
	extern void DrawRenderItems(ComPtr<ID3D12GraphicsCommandList> rCommandList);
	extern void OnResize();

	extern ComPtr<ID3D12RootSignature> rRootSignature;
	extern ComPtr<ID3DBlob> rVertexShader;
	extern ComPtr<ID3DBlob> rPixelShader;

	extern D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc;
	extern CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc;

	extern ComPtr<ID3D12PipelineState> rPso;
	extern ComPtr<ID3D12PipelineState> rPsoShadow;

	extern ComPtr<ID3D12GraphicsCommandList> rCommandList;
	extern ComPtr<ID3D12CommandAllocator> rCommandAlloc;

	extern ComPtr<ID3D12Resource> rRenderTargetBuffer[Config::frameCount];
	extern ComPtr<ID3D12Resource> rDepthStencilBuffer;

	extern INT gCurRenderTarget;
}
