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
	extern void CreateShaderResourceViews();
	extern void CreateCubemapResources();
	extern void CreateShadowMapDsvs();
	extern void Update();
	extern void UpdateAnimation();
	extern void UpdateInput();
	extern void UpdateCamera();
	extern void UpdateObjCBs(FrameResource* currentFrameResource);
	extern void UpdateMaterialCBs(FrameResource* currentFrameResource);
	extern void UpdatePassCB(FrameResource* currentFrameResource);
	extern void UpdateshadowmapCB(FrameResource* currentFrameResource);
	extern void UpdateLightCBs(FrameResource* currentFrameResource);
	extern void UpdatePipelineState();
	extern void Draw();
	extern void PopulateCommandList(FrameResource* currentFrameResource);
	extern void DrawRenderItems(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderShadowMap(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderColor(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderSkyBox(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderUI(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderCubemap(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void RenderIrradiancemap(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void SetSharedCommandListStates(ComPtr<ID3D12GraphicsCommandList> commandList);
	extern void OnResize();
	extern std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	extern ComPtr<ID3D12RootSignature> rRootSignature;
	extern ComPtr<ID3DBlob> rVertexShader;
	extern ComPtr<ID3DBlob> rPixelShader;
	extern ComPtr<ID3DBlob> rPixelPBRShader;

	extern D3D12_INPUT_LAYOUT_DESC rInputLayoutDesc;
	extern CD3DX12_DEPTH_STENCIL_DESC rDepthStencilDesc;

	extern ComPtr<ID3D12PipelineState> rPso;
	extern ComPtr<ID3D12PipelineState> rPsoShadow;
	extern ComPtr<ID3D12PipelineState> rPsoWireframe;
	extern ComPtr<ID3D12PipelineState> rPsoPBR;

	extern ComPtr<ID3D12GraphicsCommandList> rCommandList;
	extern ComPtr<ID3D12CommandAllocator> rCommandAlloc;

	extern ComPtr<ID3D12Resource> rRenderTargetBuffer[Config::numRenderTargets];
	extern ComPtr<ID3D12Resource> rDepthStencilBuffer;

	extern INT gCurBackBufferIndex;
	extern Camera gMainCam;

	extern DirectX::XMFLOAT4X4 gview;

	extern UINT objectCBVHeapIndexStart;
	extern UINT passCBVHeapIndexStart;
	extern UINT lightCBVHeapIndexStart;
	extern UINT matCBVHeapIndexStart;
	extern UINT shadowMapSRVHeapIndexStart;
	extern UINT texSRVHeapIndexStart;
	extern UINT guiSRVHeapIndexStart;

	//UI interactable parameters 
	extern int rframeRateCap60;
	extern int shaderSelector;
	extern bool wireframeMode;
	extern bool animationEnabled;
}
