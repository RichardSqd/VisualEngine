#include "pch.h"
#include "AVisualApp.h"
#include "Renderer.h"

#define NODXR false

AVisualApp::AVisualApp() {

}

void AVisualApp::InitApp() {
	//TODO: handle input 
	Graphics::Init(NODXR);
	Renderer::Init();
	

}

void AVisualApp::Update() {

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