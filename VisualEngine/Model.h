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
		UINT size;
		UINT stride;
		std::vector<size_t> data;
	public:

	};

	class SceneData {
	private:
		UINT vertexDataSize;
		UINT vertexStride;
		UINT vertexDataOffset;

		UINT indexDataSize;
		UINT indexDataOffset;
		std::vector<Model> Models;

	};

	struct TextureResource {
		UINT width;
		UINT height;
		UINT miplvl;
		DXGI_FORMAT format; 
		struct DataProperties {
			UINT offset;
			UINT size;
			UINT pitch;
			
		};
	};

	struct DrawParam {
		INT diffuseIndex;
		INT normalIndex;
		INT specularIndex;
		UINT indexStart;
		UINT indexCount;
		UINT vertexBase;

	};


}
