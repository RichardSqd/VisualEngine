#pragma once
#include "pch.h"

using Microsoft::WRL::ComPtr;
namespace Scene {
	const D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	struct Vertex {
	
		Vertex(

		): position(0.0,0.0,0.0),
			normal(0.0, 0.0, 0.0),
			tangent(0.0, 0.0, 0.0),
			texcoord(0.0, 0.0)
		{};

		Vertex(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT2& tex,
			const DirectX::XMFLOAT3& tan
		):  position(p),
			normal(n),
			tangent(tan),
			texcoord(tex){}

		Vertex(float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v): 
			position(px,py,pz),
			normal(nx, ny, nz),
			tangent(tx,ty,tz),
			texcoord(u,v){}

	
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT3 tangent;
	};

	struct Primitive {
	
		Primitive();
		
		DXGI_FORMAT iformat;
		UINT32 ibOffset;
		UINT32 indexBufferByteSize;
		UINT32 indexCount;

		UINT32 vbOffset;
		UINT32 vertexBufferByteSize;
	};



	struct Mesh {
	
		Mesh();
		
		std::vector<Primitive> primitives;
		
	};

	struct Node {
		
		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT3 translation;
		UINT32 mesh;
		UINT32 numFrameDirty;
	};

	struct Model {
	
		UINT32 numNodes;
		UINT32 numMeshes;
		UINT32 numMaterials;
		UINT32 numTextures;

		UINT32 meshDataSize;
		
		UINT32 geometrySize;

		std::vector<Mesh> meshes;
		std::vector<Node> nodes;

		std::vector<std::vector<byte>> buffers;
		ComPtr<ID3DBlob> vertexBufferCPU;
		ComPtr<ID3DBlob> indexBufferCPU;
		ComPtr<ID3D12Resource> vertexBufferGPU;
		ComPtr<ID3D12Resource> indexBufferGPU;

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView; 
		//std::unique_ptr<Mesh> mesh;
	};


	struct RenderItem {
		RenderItem() = default;
		
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 texTransform;

		Model* model;
		UINT32 meshIndex;
		std::vector<UINT32> primList;

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
