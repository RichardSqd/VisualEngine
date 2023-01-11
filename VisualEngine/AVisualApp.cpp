#include "pch.h"
#include "AVisualApp.h"
#include "Renderer.h"
#include "FrameResource.h"

#define NODXR false

AVisualApp::AVisualApp() {

}

AVisualApp::~AVisualApp() {
	if (Graphics::gDevice ) {
		Graphics::gCommandQueueManager.GetGraphicsQueue().FlushCommandQueue();
	}
}

void AVisualApp::InitApp() {
	//TODO: handle input 
	Graphics::Init(NODXR);
	auto context = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT).get();
	auto commandList = context->getCommandList();
	commandList->Reset(context->getCommandAllocator().Get(), nullptr);
	
	Renderer::Init();

	

	
	BREAKIFFAILED(commandList->Close());
	ID3D12CommandList* cmds[] = { commandList.Get() };
	auto queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
	queue.GetQueue()->ExecuteCommandLists(_countof(cmds),cmds);
	
	Time::Init();

	queue.FlushCommandQueue();


}

void AVisualApp::Update() {

	

	//update time
	Time::Tick();

	Renderer::Update();
	
	

}

void AVisualApp::Draw(void)
{

	auto queue = Graphics::gCommandQueueManager.GetGraphicsQueue();
	BREAKIFFAILED(Renderer::rCommandAlloc->Reset());


	Renderer::Draw();





	//fence move to new position and signal the new GPU fence value 
	queue.AdvanceFenceValue();
	queue.SignalFencePoint();
}

void AVisualApp::Run() {
	do {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;
	} while (EngineCore::UpdateApp());
}

void AVisualApp::ShutDown() {

}