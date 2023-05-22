#include "pch.h"
#include "AVisualApp.h"
#include "Renderer.h"
#include "FrameResource.h"
#include "Control.h"
#include "DXRRenderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "Model.h"
#include "MultithreadingContext.h"

AVisualApp::AVisualApp() {

}

AVisualApp::~AVisualApp() {
	if (Graphics::gDevice ) {
		Graphics::gCommandQueueManager.GetGraphicsQueue().FlushCommandQueue();
	}
}

void AVisualApp::InitApp() {
	//TODO: handle input 
	Graphics::Init();


	
	auto context = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT).get();
	auto commandList = context->getCommandList();
	/*CommandList Reset can be called while it's being executed
	A typical pattern is to submit a command list and then immediately reset 
	it to reuse the allocated memory for another command list.*/
	commandList->Reset(context->getCommandAllocator().Get(), nullptr);
	Renderer::Init(context);
	MultithreadingContext::CreateContexts();
	Control::InitControl(Graphics::ghWnd);
	if (Graphics::gRayTraceEnvironmentActive) {
		//DXRRenderer::Init(context);
	}

	//gui init
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(Graphics::ghWnd);

		auto fontCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap.Get()->GetCPUDescriptorHandleForHeapStart());
		fontCpuHandle.Offset(Renderer::guiSRVHeapIndexStart, Graphics::gCbvSrvUavDescriptorSize);
		auto fontGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap.Get()->GetGPUDescriptorHandleForHeapStart());
		fontGpuHandle.Offset(Renderer::guiSRVHeapIndexStart, Graphics::gCbvSrvUavDescriptorSize);

		ImGui_ImplDX12_Init(Graphics::gDevice.Get(), Graphics::gSwapChainBufferCount,
			DXGI_FORMAT_R8G8B8A8_UNORM, Graphics::gCbvSrvHeap.Get(),
			fontCpuHandle,
			fontGpuHandle);
	}

	BREAKIFFAILED(commandList->Close());
	ID3D12CommandList* cmds[] = { commandList.Get() };
	auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
	queue.GetQueue()->ExecuteCommandLists(_countof(cmds),cmds);
	
	//Time::Init();

	queue.FlushCommandQueue();
	Renderer::OnResize();


}

void AVisualApp::Update() {

	bool show_demo_window = true;

	//update time
	Time::Tick();
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow(&show_demo_window);
	//ImGui::ShowMetricsWindow();

	UpdateUI();
	Renderer::Update();
	
	

}

void AVisualApp::UpdateUI() {
	ImGuiIO& io = ImGui::GetIO();
	

	auto& sceneLighting = EngineCore::eModel.lights;
	auto& axis = Scene::axis;
	auto& angle = Scene::angle;
	auto& translation = Scene::sceneTranslation;
	auto& scaling = Scene::sceneScaling;
	auto frameIndex = Graphics::gFrameResourceManager.GetCurrentIndex();
	bool wireframeMode = Renderer::wireframeMode;
	bool animationEnabled = Renderer::animationEnabled;
	bool limitFrameRate = Renderer::rframeRateCap60;
	{
		static float f = 0.0f;
		static int counter = 0;
		
		ImGui::Begin("Welcome to Visual Engine!");                          
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::Checkbox("Limit Frame Rate to 60 FPS", &limitFrameRate);
		ImGui::Text("Current window size (%.1f, %.1f) ", Graphics::gWidth, Graphics::gHeight);
		ImGui::Text("Camera Position %.3f %.3f %.3f (x,y,z) ", Renderer::gMainCam.camPos.x, Renderer::gMainCam.camPos.y, Renderer::gMainCam.camPos.z);
		ImGui::Checkbox("Wireframe Mode", &wireframeMode);
		ImGui::Checkbox("Animation Start", &animationEnabled);

		if (sceneLighting.numDirectionalLights > 0) {
			ImGui::SeparatorText("Directional Lights");
			for (int i = 0; i < sceneLighting.numDirectionalLights; i++) {
				auto& dl = sceneLighting.directionalLights[i];
				float lastx = dl.strength.x;
				float lasty = dl.strength.y;
				float lastz = dl.strength.z;

				//ImGui::InputFloat3("input float3", vec4f);

				ImGui::SliderFloat("strength x", &dl.strength.x, 0.0f, 1.0f, "ratio = %.3f");
				ImGui::SliderFloat("strength y", &dl.strength.y, 0.0f, 1.0f, "ratio = %.3f");
				ImGui::SliderFloat("strength z", &dl.strength.z, 0.0f, 1.0f, "ratio = %.3f");
				if (lastx != dl.strength.x || lasty != dl.strength.y || lastz != dl.strength.z) {
					EngineCore::eModel.lightnumFrameDirty = Config::numFrameResource;
				}
			}
		}

		ImGui::Text("Scene Scaling");
		float lastx = scaling.x;
		float lasty = scaling.y;
		float lastz = scaling.z;
		ImGui::SliderFloat("Scaling x", &scaling.x, 0.0f, 20.0f, "ratio = %.3f");
		ImGui::SliderFloat("Scaling y", &scaling.y, 0.0f, 20.0f, "ratio = %.3f");
		ImGui::SliderFloat("Scaling z", &scaling.z, 0.0f, 20.0f, "ratio = %.3f");

		ImGui::Text("Shader Settings");

		//if (ImGui::BeginMenu("Shader Settings"))
		//{
			//static bool enabled = true;
			//ImGui::MenuItem("Enabled", "", &enabled);

			//static int n = 0;
			//ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
			//ImGui::InputFloat("Input", &f, 0.1f);
			ImGui::Combo("Shader selector", &Renderer::shaderSelector, "Phong\0Blinn-Phong\0PBR\0Hybrid Raytracing\0\0");
			
			//ImGui::EndMenu();
		//}
		ImGui::Text("HDR IBL INPUT");
		auto skyTextureHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		ImGui::Image((ImTextureID)skyTextureHandle.ptr, ImVec2((float)200, (float)100));
		
		
		//if (ImGui::BeginMenu("Cubemap details"))
		{
			ImGui::Text("Processed Cubemaps");
			auto h = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

			for (int i = 0; i < 6; i++) {
				h.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
				ImGui::Image((ImTextureID)h.ptr, ImVec2((float)200, (float)200));
			}

			
			//ImGui::EndMenu();
		}

		
		if (ImGui::BeginMenu("Irradiance maps"))
		{

			auto h = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
			h.Offset(6, Graphics::gCbvSrvUavDescriptorSize);

			for (int i = 0; i < 6; i++) {
				h.Offset(1, Graphics::gCbvSrvUavDescriptorSize);
				ImGui::Image((ImTextureID)h.ptr, ImVec2((float)200, (float)200));
			}


			ImGui::EndMenu();
		}

		//if (ImGui::BeginMenu("shadow map"))
		//{
		//	auto h = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics::gCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
		//	h.Offset(Renderer::shadowMapSRVHeapIndexStart + frameIndex, Graphics::gCbvSrvUavDescriptorSize);
		//	ImGui::Image((ImTextureID)h.ptr, ImVec2((float)200, (float)200));
			
		//	ImGui::EndMenu();
		//}
		
		
		// start required updates 
		if (lastx != scaling.x || lasty != scaling.y || lastz != scaling.z) {
			for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
				EngineCore::eModel.nodes[i].numFrameDirty = Config::numFrameResource;
			}
		}

		if (limitFrameRate != (bool)Renderer::rframeRateCap60) {
			Renderer::rframeRateCap60 = limitFrameRate;
		}

		if (wireframeMode != Renderer::wireframeMode ) {
			Renderer::wireframeMode = wireframeMode;
		}

		if (animationEnabled != Renderer::animationEnabled) {
			Renderer::animationEnabled = animationEnabled;
			Time::tAnimationTimer = 0.0;
			if (animationEnabled == false) {
				for (auto& node : EngineCore::eModel.nodes) {
					//DirectX::XMStoreFloat4x4(&node.animation.animatedMatrix, DirectX::XMMatrixIdentity());
					node.numFrameDirty = Config::numFrameResource;
				}
			
			}
		}

		ImGui::End();
	}
}

void AVisualApp::Draw(void)
{

	
	ImGui::Render();
	Renderer::Draw();





	
}

void AVisualApp::Run() {

	do {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				//__debugbreak();
				return;
			}
		}

			
	} while (EngineCore::UpdateApp());
}

void AVisualApp::ShutDown() {
	auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();

	queue.FlushCommandQueue();
	//CloseHandle(Graphics::ghWnd);
}