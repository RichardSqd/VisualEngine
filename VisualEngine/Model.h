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

		) {};

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
		
		DXGI_FORMAT format;
		int indexBuffer;
		UINT32 indexCount;
		UINT32 ibOffset;

		int vertexBuffer;
		UINT32 beginVertex;
		UINT32 vbOffset;
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
		UINT mesh;
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

		//std::unique_ptr<Mesh> mesh;
	};


	struct RenderItem {
		RenderItem() = default;
		
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 texTransform;

		Model* model;
		

		//
		UINT indexNum;
		UINT indexPos;
		UINT vertexPos;

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
