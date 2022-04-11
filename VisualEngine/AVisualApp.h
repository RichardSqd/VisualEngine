#pragma once
#include "EngineCore.h"

class AVisualApp : public EngineCore::IVisualApp {
public:
	AVisualApp(void);
	void InitApp(void) override;
	void Update(void) override;
	void ShutDown(void) override;
	~AVisualApp()
	{

	}

private:


};