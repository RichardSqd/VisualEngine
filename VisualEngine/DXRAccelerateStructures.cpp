#include "pch.h"
#include "DXRAccelerateStructures.h"

namespace DXRAS {
	inline ComPtr<ID3D12Resource> CreateBuffer(
		ID3D12Device* device,
		UINT64 size,
		D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initState,
		D3D12_HEAP_PROPERTIES& heapProps);

	BottomLevelAS::BottomLevelAS()
	{
	}
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
								bool allowUpdate)
	{
		for (UINT i = 0; i < count; i++) {

			D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
			desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			desc.Triangles.VertexBuffer.StartAddress = vertexBuffers[i]->GetGPUVirtualAddress() + vertexOffsets[i];
			desc.Triangles.VertexBuffer.StrideInBytes = vertexStrideInByte;
			desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			desc.Triangles.IndexBuffer = indexBuffers[i]->GetGPUVirtualAddress() + indexOffsets[i];
			desc.Triangles.IndexCount = indexCount[i];
			desc.Triangles.IndexFormat = indexFormat;
			desc.Triangles.Transform3x4 =
				transformations[i] ?
				(transformations[i]->GetGPUVirtualAddress() + transformOffsets[i]) :
				0;
			desc.Flags = opacity[i]
				? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE
				: D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

			m_VBdescriptors.push_back(desc);

		}

		m_flags = allowUpdate ?
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE :
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		//compute byte size needed for buffers 
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputDesc = {};
			inputDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputDesc.NumDescs = m_VBdescriptors.size();
			inputDesc.pGeometryDescs = m_VBdescriptors.data();
			inputDesc.Flags = m_flags;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildDesc = {};
			device->GetRaytracingAccelerationStructurePrebuildInfo(&inputDesc, &prebuildDesc);
			m_scratchSizeInBytes = Math::Alignment256(prebuildDesc.ScratchDataSizeInBytes);
			m_resultSizeInBytes = Math::Alignment256(prebuildDesc.ResultDataMaxSizeInBytes);

		}

		//allocate gpu buffers 
		pScratch = CreateBuffer(device.Get(), m_scratchSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
		pResult = CreateBuffer(device.Get(), m_resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
		
	}

	void BottomLevelAS::CreateBLAS( ID3D12GraphicsCommandList4* commandList,
									ComPtr<ID3D12Resource> previousResult,
								    bool updateOnly) {
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = m_flags;

		if (m_flags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE &&
			updateOnly) {
			flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		if (m_flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE &&
			updateOnly) {
			Utils::Print("update failing,  not updateble AS");
			__debugbreak();
		}

		if (updateOnly && previousResult == nullptr) {
			Utils::Print("update failing, previous result not provided");
			__debugbreak();
		}

		if (m_resultSizeInBytes == 0 || m_scratchSizeInBytes == 0) {
			Utils::Print("zero byte size");
			__debugbreak();
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		desc.Inputs.NumDescs = m_VBdescriptors.size();
		desc.Inputs.pGeometryDescs = m_VBdescriptors.data();
		desc.DestAccelerationStructureData = {
			pResult->GetGPUVirtualAddress()
		};
		desc.ScratchAccelerationStructureData = {
			pScratch->GetGPUVirtualAddress()
		};
		desc.SourceAccelerationStructureData = previousResult ?
			previousResult->GetGPUVirtualAddress() 
			: 0;
		desc.Inputs.Flags = flags;

		commandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

		//wait for builder to complete by setting a berrier on the resulting buffer

		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResult.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		commandList->ResourceBarrier(1, &uavBarrier);


	}

	TopLevelAS::TopLevelAS()
	{
	}

	void TopLevelAS::InitTLAS(
							ComPtr<ID3D12Device5> device,
							UINT count,
							std::vector<ComPtr< ID3D12Resource>> blases,
							std::vector < DirectX::XMMATRIX> transforms,
							std::vector<UINT> instanceIDs,
							std::vector<UINT> hitGrupIndices,
							bool allowUpdate) {
		
		for (UINT i = 0; i < count; i++) {
			m_instances.emplace_back(Instance(blases[i].Get(), transforms[i], instanceIDs[i], hitGrupIndices[i]));
		}
		
		
		
		m_flags =	allowUpdate ? 
					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE :
					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		
		//compute byte size needed for buffers 
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputDesc = {};
			inputDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			inputDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputDesc.NumDescs = m_instances.size();
			inputDesc.Flags = m_flags;


			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildDesc = {};
			device->GetRaytracingAccelerationStructurePrebuildInfo(&inputDesc, &prebuildDesc);
			m_scratchSizeInBytes = Math::Alignment256(prebuildDesc.ScratchDataSizeInBytes);
			m_resultSizeInBytes = Math::Alignment256(prebuildDesc.ResultDataMaxSizeInBytes);
			m_instanceDescriptorSizeInBytes = Math::Alignment256(
											static_cast<UINT64>(sizeof(D3D12_RAYTRACING_INSTANCE_DESC)) * 
											static_cast<UINT64>(m_instances.size()));
		
		}

		pScratch = CreateBuffer(device.Get(), m_scratchSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
		pResult = CreateBuffer(device.Get(), m_resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
		pInstanceDesc = CreateBuffer(device.Get(), m_instanceDescriptorSizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD));
		
	}

	void TopLevelAS::CreateTLAS(
							ID3D12GraphicsCommandList4* commandList, 
							ComPtr<ID3D12Resource> previousResult,
							bool updateOnly) {
		
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = {};
		pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
		
		if (instanceDescs == nullptr) {
			Utils::Print("map instance desc failing");
			__debugbreak();
		}
		if (!updateOnly) {
			ZeroMemory(instanceDescs, m_instanceDescriptorSizeInBytes);
		}

		//create instance descriptors 
		
		for (UINT i = 0; i < m_instances.size(); i++) {
			instanceDescs[i].InstanceID = m_instances[i].instanceID;
			instanceDescs[i].InstanceContributionToHitGroupIndex = m_instances[i].hitGrupIndex;
			instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			memcpy(&(instanceDescs[i].Transform), &m_instances[i].transform, sizeof(m_instances[i].transform));
			instanceDescs[i].AccelerationStructure = m_instances[i].blas->GetGPUVirtualAddress();
			instanceDescs[i].InstanceMask = 0xFF;

		}

		pInstanceDesc->Unmap(0,nullptr);

		const D3D12_GPU_VIRTUAL_ADDRESS pSourceAS = updateOnly ? previousResult->GetGPUVirtualAddress() : 0;
		
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = m_flags;
		if (m_flags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE &&
			updateOnly) {
			flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		}

		if (m_flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE &&
			updateOnly) {
			Utils::Print("update failing,  not updateble AS");
			__debugbreak();
		}

		if (updateOnly && previousResult == nullptr) {
			Utils::Print("update failing, previous result not provided");
			__debugbreak();
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		desc.Inputs.InstanceDescs = pInstanceDesc->GetGPUVirtualAddress();
		desc.Inputs.NumDescs = m_instances.size();
		desc.DestAccelerationStructureData = {
			pResult->GetGPUVirtualAddress()
		};
		desc.ScratchAccelerationStructureData = {
			pScratch->GetGPUVirtualAddress()
		};
		desc.SourceAccelerationStructureData = previousResult ?
			previousResult->GetGPUVirtualAddress()
			: 0;

		desc.Inputs.Flags = flags;

		commandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);



		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResult.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		commandList->ResourceBarrier(1, &uavBarrier);


	}

	inline ComPtr<ID3D12Resource> CreateBuffer(
											ID3D12Device* device,
											UINT64 size,
											D3D12_RESOURCE_FLAGS flags, 
											D3D12_RESOURCE_STATES initState,
											D3D12_HEAP_PROPERTIES& heapProps) {
		D3D12_RESOURCE_DESC bufDesc = {};
		bufDesc.Alignment = 0;
		bufDesc.DepthOrArraySize = 1;
		bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufDesc.Flags = flags;
		bufDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufDesc.Height = 1;
		bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufDesc.MipLevels = 1;
		bufDesc.SampleDesc.Count = 1;
		bufDesc.SampleDesc.Quality = 0;
		bufDesc.Width = size;

		ComPtr<ID3D12Resource> resource;
		BREAKIFFAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufDesc, initState, nullptr, IID_PPV_ARGS(&resource)));
		return resource;
	}
}