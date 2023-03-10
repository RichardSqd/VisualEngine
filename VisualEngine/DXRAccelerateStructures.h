#pragma once
#include "pch.h"

namespace DXRAS {
	class BottomLevelAS {
	public:
		BottomLevelAS();
		void BottomLevelAS::InitBLAS(
			ComPtr<ID3D12Device5> device,
			UINT count,
			std::vector<ComPtr<ID3D12Resource>> vertexBuffers,
			std::vector<UINT> vertexOffsets,
			std::vector<ComPtr<ID3D12Resource>> transformations,
			std::vector<UINT64> transformOffsets,
			UINT vertexStrideInByte,
			std::vector<ComPtr<ID3D12Resource>> indexBuffers,
			std::vector<UINT> indexOffsets,
			std::vector <UINT> indexCount,
			DXGI_FORMAT indexFormat,
			std::vector<bool> opacity,
			bool allowUpdate);

		void BottomLevelAS::CreateBLAS(
									ID3D12GraphicsCommandList4* commandList,
									ComPtr<ID3D12Resource> previousResult,
									bool updateOnly);

	private:
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_VBdescriptors = {};
		UINT64 m_scratchSizeInBytes = 0;
		UINT64 m_resultSizeInBytes = 0;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_flags = {};


		ComPtr<ID3D12Resource> pScratch;
		ComPtr<ID3D12Resource> pResult;
		//ComPtr<ID3D12Resource> pInstanceDesc;

	};

	class TopLevelAS {
	public:
		TopLevelAS();
		
		void TopLevelAS::InitTLAS(
			ComPtr<ID3D12Device5> device,
			UINT count,
			std::vector<ComPtr< ID3D12Resource>> blases,
			std::vector < DirectX::XMMATRIX> transforms,
			std::vector<UINT> instanceIDs,
			std::vector<UINT> hitGrupIndices,
			bool allowUpdate);

		void TopLevelAS::CreateTLAS(
			ID3D12GraphicsCommandList4* commandList,
			ComPtr<ID3D12Resource> previousResult,
			bool updateOnly);
	private:
		struct Instance {
			Instance(ID3D12Resource* blases, DirectX::XMMATRIX trans, UINT iId, UINT hgId) :
				blas(blas), transform(trans), instanceID(iId), hitGrupIndex(hgId) {}
			ID3D12Resource* blas;
			DirectX::XMMATRIX transform;
			UINT instanceID;
			UINT hitGrupIndex;
		};

		std::vector<Instance> m_instances;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_flags = {};
		
		UINT64 m_scratchSizeInBytes = 0;
		UINT64 m_resultSizeInBytes = 0;
		UINT64 m_instanceDescriptorSizeInBytes = 0;

		ComPtr<ID3D12Resource> pScratch;
		ComPtr<ID3D12Resource> pResult;
		ComPtr<ID3D12Resource> pInstanceDesc; //matrices of instances
	};
}