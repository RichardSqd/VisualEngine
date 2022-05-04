#pragma once

namespace Scene {
	const D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};


	class Model {
	private:
		size_t size; 
		size_t stride;
		std::vector<size_t> data;
	public:

	};

	class SceneData {
	private:
		size_t vertexDataSize;
		size_t vertexStride;
		size_t vertexDataOffset;

		size_t indexDataSize;
		size_t indexDataOffset;
		std::vector<Model> Models;

	};
}
