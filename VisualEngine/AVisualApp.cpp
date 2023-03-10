#include "pch.h"
#include "AVisualApp.h"
#include "Renderer.h"
#include "FrameResource.h"
#include "Control.h"
#include "DXRRenderer.h"


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
	
	Control::InitControl(Graphics::ghWnd);
	auto context = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT).get();
	auto commandList = context->getCommandList();
	/*CommandList Reset can be called while it's being executed
	A typical pattern is to submit a command list and then immediately reset 
	it to reuse the allocated memory for another command list.*/
	commandList->Reset(context->getCommandAllocator().Get(), nullptr);
	Renderer::Init(context);
	if (Graphics::gRayTraceEnvironmentActive) {
		//DXRRenderer::Init(context);
	}
	BREAKIFFAILED(commandList->Close());
	ID3D12CommandList* cmds[] = { commandList.Get() };
	auto& queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
	queue.GetQueue()->ExecuteCommandLists(_countof(cmds),cmds);
	
	Time::Init();

	queue.FlushCommandQueue();
	Renderer::OnResize();


}

void AVisualApp::Update() {

	

	//update time
	Time::Tick();

	Renderer::Update();
	
	

}

void AVisualApp::Draw(void)
{

	

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