#pragma once
#include "SceneLoader.h"
#include "Graphics.h"
#include "EngineCore.h"
#include "Camera.h"

namespace Renderer {
	extern void Init(CommandContext* context);
	extern void InitCamera();
	extern void CreateRootSigniture();
	extern void CreatePipelineState();
	extern void CreateShadersAndInputLayout();
	extern void CreateRTVDSV();
	extern void CreateDescriptorHeaps();
	extern void CreateFrameResources();
	extern void CreateSwapChain();
	extern void CreateConstantBufferViews();
	extern void Update();
	extern void UpdateInput();
	extern void UpdateCamera();
	extern void UpdateObjCBs(FrameResource* currentFrameResource);
	extern void UpdatePassCB(FrameResource* currentFrameResource);
	extern void Draw();
	extern void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void DrawRenderItems(ComPtr<ID3D12GraphicsCommandList> commandList);
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

	extern INT gCurBackBufferIndex;
	extern Camera gMainCam;

	extern DirectX::XMFLOAT4X4 gview;
}
