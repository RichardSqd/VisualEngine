#pragma once
#include "EngineCore.h"



class AVisualApp : public EngineCore::IVisualApp {
public:
	AVisualApp(void);
	void InitApp(void) override;

	void UpdateUI();
	void Update(void) override;
	void Draw(void) override;
	void Run(void) override;
	void ShutDown(void) override;
	~AVisualApp(void);

private:
	

};