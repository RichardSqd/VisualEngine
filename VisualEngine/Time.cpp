#include "pch.h"
#include "Time.h"
#include "Graphics.h"
#include "imgui.h"

namespace Time {
	double tBaseTime;
	double tCurTime;
	double tTimeElapsed;
	double tDeltaTime;
	double tFPS;
	double tFreq;
	int tNumFrames;

	double tAnimationTimer;

	void Init() {

		LARGE_INTEGER freq;
		ASSERT(QueryPerformanceCounter(&freq));
		tFreq = 1.0 / (double)freq.QuadPart;
		tBaseTime = 0.0;
		tCurTime = 0.0;
		tTimeElapsed = 0.0;
		tNumFrames = 0;
		tAnimationTimer = 0.0;
		
		
	}

	void Start() {
		LARGE_INTEGER start;

		ASSERT(QueryPerformanceCounter(&start));
		tBaseTime = (double)start.QuadPart;
	}

	void Reset() {
		LARGE_INTEGER start;
		ASSERT(QueryPerformanceCounter(&start));
		tBaseTime = (double)start.QuadPart;
	}

	void CalcFPS() {
		//tNumFrames++;

		/*if (tTimeElapsed - tDeltaTime >= 1.0) {
			tFPS = tNumFrames;
			tDeltaTime += 1.0;
			
			
			
			tNumFrames = 0;
			tTimeElapsed += 1.0;
		}*/
	}
	void Tick() {
		//LARGE_INTEGER cur;
		//ASSERT(QueryPerformanceCounter(&cur));
		//tCurTime = (double)cur.QuadPart;
		//tTimeElapsed = (tCurTime - tBaseTime) * tFreq;
		ImGuiIO& io = ImGui::GetIO();
		std::wstring fpsText =L"Framerate: " + std::to_wstring(round(io.Framerate*100)/100).substr(0,4) + L" FPS";
		SetWindowText(Graphics::ghWnd, fpsText.c_str());

		//CalcFPS();

	}

	
	

}