#include "pch.h"
#include "Renderer.h"


namespace Renderer {
	Scene::Model mModel;

	void Init() {
		ASSERT(Scene::LoadScene(Config::filePath, mModel));
	}
}