#include "pch.h"
#include "DXRRenderer.h"
#include "EngineCore.h"
#include "DXRAccelerateStructures.h"

namespace DXRRenderer {
	DXRAS::BottomLevelAS blasBuilder = {};
	DXRAS::TopLevelAS tlasBuilder = {};
	void CreateAccelerateStructures(ComPtr<ID3D12Device5> device, ComPtr < ID3D12GraphicsCommandList4> commandList) {
		Scene::Model& model = EngineCore::eModel;
		UINT objCount = model.numNodes;
		UINT matCount = model.numMaterials;

		UINT count = 0 ;
		std::vector<ComPtr<ID3D12Resource>> vertexBuffers;
		std::vector<UINT> vertexOffsets;
		std::vector<ComPtr<ID3D12Resource>> transformations;
		std::vector<UINT64> transformOffsets ;
		std::vector<ComPtr<ID3D12Resource>> indexBuffers;
		std::vector<UINT> indexOffsets;
		std::vector <UINT> indexCount;
		std::vector<bool> opacity;

		model.indexBufferGPU->GetGPUVirtualAddress();

		for (UINT i = 0; i < EngineCore::eModel.numNodes; i++) {
			auto& node = model.nodes[i];
			auto& primitives = model.meshes[node.mesh].primitives;
			indexBuffers.push_back(model.indexBufferGPU);
			
			//for (auto& prim : primitives) {
				
			//}
		}


		//blasBuilder.InitBLAS(device, model.numNodes, )

	}

	void Init(CommandContext* context) {
		
		auto dxrCommandList = context->getDXRCommandList();

		
		if (Graphics::gRayTraceEnvironmentActive) {
			BREAKIFFAILED(Graphics::gDevice->QueryInterface(IID_PPV_ARGS(Graphics::gDXRDevice.GetAddressOf())));
			DXRRenderer::CreateAccelerateStructures(Graphics::gDXRDevice, dxrCommandList);
		}


	}

	void Destroy() {

	}

	
	
	
}