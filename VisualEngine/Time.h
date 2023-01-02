#pragma once


namespace Time{
	extern void Init();
	extern void Start();
	extern void Reset();
	extern void Tick();

	extern void CalcFPS();



	extern double tBaseTime;
	extern double tCurTime;
	extern double tTimeElapsed;
	extern double tDeltaTime;
	extern double tFPS;
	extern double tFreq;
	extern int tNumFrames;
	
}