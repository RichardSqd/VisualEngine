#pragma once
#include "pch.h"
#include "Material.h"
#include <unordered_map>
#include <string>

using Microsoft::WRL::ComPtr;
namespace Scene {
	/*
	const D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,     0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};*/

	const D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0, DXGI_FORMAT_R32G32_FLOAT,     2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	struct VertexPos 
	{
		DirectX::XMFLOAT3 position;
	};

	struct VertexNormal
	{
		DirectX::XMFLOAT3 normal;
	};

	struct VertexTexCord
	{
		DirectX::XMFLOAT2 texcoord;
	};

	struct VertexTangent
	{
		DirectX::XMFLOAT3 tangent;
	};

	struct VertexColor
	{
		DirectX::XMFLOAT4 color;
	};

	struct Vertex2
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	struct Vertex {
	
		Vertex(

		) : position(0.0, 0.0, 0.0),
			normal(0.0, 0.0, 0.0),
			texcoord(0.0, 0.0),
			tangent(0.0, 0.0, 0.0),
			color(0.0, 0.0, 0.0, 0.0)
			
		{};

		Vertex(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT2& tex,
			const DirectX::XMFLOAT3& tan,
			const DirectX::XMFLOAT4& color
		):  position(p),
			normal(n),
			tangent(tan),
			texcoord(tex),
			color(color){}

		Vertex(float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v,
			float r, float g, float b, float a): 
			position(px,py,pz),
			normal(nx, ny, nz),
			tangent(tx,ty,tz),
			texcoord(u,v),
			color(r,g,b,a){}

	
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT4 color;
	};

	struct Primitive {
	
		Primitive();
		
		DXGI_FORMAT iformat;
		UINT ibOffset;
		UINT indexBufferByteSize;
		UINT indexCount;

		UINT vbPosOffset;
		UINT vertexBufferPosByteSize;
		UINT vertexCount;

		UINT vbTexOffset; 
		UINT vertexBufferTexCordByteSize;

		////D3D12_VERTEX_BUFFER_VIEW vertexPosBufferView{};
		//D3D12_VERTEX_BUFFER_VIEW vertexNormalBufferView{};
		//D3D12_VERTEX_BUFFER_VIEW vertexTexCordBufferView{};
		//D3D12_VERTEX_BUFFER_VIEW vertexTangentBufferView{};
		//D3D12_VERTEX_BUFFER_VIEW vertexColorBufferView{};
		//D3D12_INDEX_BUFFER_VIEW indexBufferView{};

		UINT matIndex;
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

		UINT32 indexCount;
		UINT32 ibOffset;
		UINT32 vbOffset;

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
		std::vector<std::unique_ptr<Material>> materials;

		std::unordered_map<std::string, std::unique_ptr<Texture>> textures;

		//std::vector<std::vector<byte>> buffers;
		std::vector<byte> vertexPosBufferCPU;
		std::vector<byte> vertexNormalBufferCPU;
		std::vector<byte> vertexTexCordBufferCPU;
		std::vector<byte> vertexTangentBufferCPU;
		std::vector<byte> vertexColorBufferCPU;

		std::vector<byte> indexBufferCPU;

		ComPtr<ID3D12Resource> vertexPosBufferGPU;
		ComPtr<ID3D12Resource> vertexNormalBufferGPU;
		ComPtr<ID3D12Resource> vertexTexCordBufferGPU;
		ComPtr<ID3D12Resource> vertexTangentBufferGPU;
		ComPtr<ID3D12Resource> vertexColorBufferGPU;
		ComPtr<ID3D12Resource> indexBufferGPU;


		UINT numPrimitives = 0;
		long long indexBufferByteSize;
		long long vertexPosBufferByteSize;

		long long vertexTexCordBufferByteSize;

		D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
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
