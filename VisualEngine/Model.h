#pragma once

namespace Scene {
	const D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	class Vertex {
	public:
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

	private:
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT3 tangent;
	};

	class Primitives {
	public:
		Primitives();
		
		DXGI_FORMAT format;
		int indexBuffer;
		UINT32 indexCount;
		UINT32 ibOffset;

		int vertexBuffer;
		UINT32 beginVertex;
		UINT32 vbOffset;
	};



	class Mesh {
	public:
		Mesh();
		
		std::vector<Primitives> primitives;
		
	};

	class Node {
	public:
		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT3 translation;
		UINT mesh;
	};

	class Model {
	public:
		UINT32 numNodes;
		UINT32 numMeshes;
		UINT32 numMaterials;
		UINT32 numTextures;

		UINT32 meshDataSize;
		
		UINT32 geometrySize;

		std::vector<Mesh> meshes;
		std::vector<Node> nodes;

		std::vector<std::vector<byte>> buffers;


		//std::unique_ptr<Mesh> mesh;
	};


	class RenderItem {
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
