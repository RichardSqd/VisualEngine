#include "pch.h"
#include "SceneLoader.h"
#include "Graphics.h"
#include "tiny_gltf.h"


using Microsoft::WRL::ComPtr;

namespace Scene {

	ComPtr<ID3D12Resource> vertexUploader;
	ComPtr<ID3D12Resource> indexUploader;

	void SolveMeshs(tinygltf::Model& tinyModel, int meshIndex , Scene::Model& model, DirectX::XMMATRIX& localToObject) {
		
		Mesh& mesh = model.meshes[meshIndex] = Mesh{};
		tinygltf::Mesh& tinyMesh = tinyModel.meshes[meshIndex];

		/*
		for (UINT i = 0; i < tinyModel.bufferViews.size(); i++) {
			const tinygltf::BufferView& tinyBufferView = tinyModel.bufferViews[i];
			if (tinyBufferView.target == 0) {
				continue;
			}

			const tinygltf::Buffer& tinyBuffer = tinyModel.buffers[tinyBufferView.buffer];
			size_t size = tinyBuffer.data.size();
			size_t offset = tinyBufferView.byteOffset;

			Utils::Print(("\ntarget:"+ tinyBufferView.name+"size:" + std::to_string(size) + ", offset:" + std::to_string(offset) + "\n").c_str());
		}*/

		mesh.primitives.resize(tinyMesh.primitives.size());
		for (UINT i = 0; i < tinyMesh.primitives.size(); i++) {
			tinygltf::Primitive& tinyPrimitive = tinyMesh.primitives[i];
			Primitive& primitive = mesh.primitives[i];
			{ //Index buffer
				tinygltf::Accessor indexAccessor = tinyModel.accessors[tinyPrimitive.indices];
				tinygltf::BufferView& indexBufferview =  tinyModel.bufferViews[indexAccessor.bufferView];

				ASSERT(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
				primitive.ibOffset = indexBufferview.buffer;
				primitive.iformat = DXGI_FORMAT_R16_UINT;
				primitive.indexCount = indexAccessor.count;
				primitive.ibOffset = indexAccessor.byteOffset + indexBufferview.byteOffset;
			}


			for (auto& attribute : tinyPrimitive.attributes) {
				tinygltf::Accessor accessor = tinyModel.accessors[attribute.second];
				tinygltf::BufferView vaBufferview = tinyModel.bufferViews[accessor.bufferView];

				int byteStride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);
				size_t byteOffset = accessor.byteOffset;


				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				ASSERT(vaa > 0);
				if (attribute.first.compare("POSITION") == 0) {
					//TODO: apply matrix 
					vaa = 0;
					 
				} 
				else if (attribute.first.compare("NORMAL") == 0) {
					vaa = 1;
				} 
				else if (attribute.first.compare("TEXCOORD_0") == 0) {
					vaa = 2;
				}
				else if (attribute.first.compare("TANGENT") == 0) {
					vaa = 3;
				}
				Utils::Print(attribute.first.c_str());
				

				switch (accessor.componentType)
				{
					case TINYGLTF_COMPONENT_TYPE_BYTE:
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						break;
					case TINYGLTF_COMPONENT_TYPE_SHORT:
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						break;
					case TINYGLTF_COMPONENT_TYPE_INT:
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						break;
					case TINYGLTF_COMPONENT_TYPE_FLOAT:
						break;
					case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						break;
					default:
						break;
				}

				
					
				
			}

			if (tinyModel.textures.size() > 0) {
				tinygltf::Texture& tex = tinyModel.textures[0];

				if (tex.source > -1) {
					tinygltf::Image& image = tinyModel.images[tex.source];
					
					
					if (image.component == 1) {
						//R
					}
					else if (image.component == 2) {
						//RG
					}
					else if (image.component == 3) {
						//RGB
					}
					else {
						// ???
					}

				}
			}

		}


	}

	//recursively discover nodes 
	void SolveNodes(tinygltf::Model& tinyModel, int nodeIndex, Scene::Model& model, DirectX::XMMATRIX& localToObject) {

		model.nodes[nodeIndex] = Node{};
		tinygltf::Node& tinyNode = tinyModel.nodes[nodeIndex];
		//validate current mesh
		
		if ((tinyNode.mesh >= 0) && (tinyNode.mesh < tinyModel.meshes.size())) {

			if (tinyNode.matrix.size() != 0) {
				localToObject = Math::VectorToMatrix(tinyNode.matrix) * localToObject;
			}
			else {
				DirectX::XMMATRIX scale = Math::Scale(tinyNode.scale);
				DirectX::XMMATRIX translate = Math::Trans(tinyNode.translation);
				DirectX::XMMATRIX rotate = Math::QuaternionToMatrix(tinyNode.rotation);
				DirectX::XMMATRIX m = translate * rotate * scale;
				localToObject *= m;
				
			}
			DirectX::XMStoreFloat4x4(&model.nodes[nodeIndex].matrix, localToObject);

			SolveMeshs(tinyModel, tinyNode.mesh, model, localToObject);
		}

		for (UINT i = 0; i < tinyNode.children.size(); i++) {
			ASSERT((tinyNode.children[i] >= 0) && (tinyNode.children[i] < tinyModel.nodes.size()));
			SolveNodes(tinyModel, tinyNode.children[i], model, localToObject);
		}

		

	}

	int translate(tinygltf::Model& tinyModel, Scene::Model& model) {
		const tinygltf::Scene& scene = tinyModel.scenes[tinyModel.defaultScene];
		model.buffers.resize(tinyModel.buffers.size());
		model.meshes.resize(tinyModel.meshes.size());
		model.nodes.resize(tinyModel.nodes.size());

		model.numNodes = tinyModel.nodes.size();
		model.numMeshes = tinyModel.meshes.size();
		model.numMaterials = tinyModel.materials.size();
		model.numTextures = tinyModel.textures.size();

		//DirectX::XMLoadFloat4x4(model.);

		//move over all buffers 
		for (UINT i = 0; i < tinyModel.buffers.size(); i++) {
			model.buffers[i] = std::move(tinyModel.buffers[i].data);
		}

		//iterate over all nodes 
		for (UINT i = 0; i < scene.nodes.size(); i++) {
			SolveNodes(tinyModel, scene.nodes[i], model, Math::IdentityMatrix());
		}

		return 1;
	}



	int LoadScene(std::wstring filename, Model& model) {
		
		tinygltf::Model tinyModel;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string filenamebyte = Utils::to_byte_str(filename);
		std::string ext = tinygltf::GetFilePathExtension(filenamebyte);

		bool store_original_json_for_extras_and_extensions = false;
		loader.SetStoreOriginalJSONForExtrasAndExtensions(
			store_original_json_for_extras_and_extensions);


		bool ret = false;
		if (ext.compare("glb") == 0) {
			Utils::Print("Reading binary glTF\n");
			// assume binary glTF.
			ret = loader.LoadBinaryFromFile(&tinyModel, &err, &warn,
				filenamebyte.c_str());
		}
		else {
			Utils::Print("Reading ASCII glTF\n");
			// assume ascii glTF.
			ret =
				loader.LoadASCIIFromFile(&tinyModel, &err, &warn, filenamebyte.c_str());
		}

		if (!warn.empty()) {
			Utils::Print(warn.c_str());
		}

		if (!err.empty()) {
			Utils::Print(err.c_str());
		}

		if (!ret) {
			Utils::Print("Failed to parse glTF\n");
			return 0;
		}
		Utils::Print(tinyModel.meshes[0].name.c_str());
		translate(tinyModel, model);
		return 1;
	}

	int LoadTestScene(std::wstring filename, Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {

		std::string filenamebyte = Utils::to_byte_str(filename);
		std::ifstream fin(filenamebyte);
		if (!fin) {
			Utils::Print("Test File Not Found");
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;


		std::vector<Vertex> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i) {
			fin >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
			fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;
		}

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		std::vector<std::int32_t> indices(3 * tcount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);


		auto prim = Primitive{};
		
		

		//Graphics::gDevice->CreateCommittedResource(
		//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		//	D3D12_HEAP_FLAG_NONE,
		//	&CD3DX12_RESOURCE_DESC::Buffer(vbByteSize),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(&model.vertexBuffer));
		
		//BREAKIFFAILED(D3DCreateBlob(vbByteSize, &model.vertexBufferCPU));
		//memcpy(model.vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		//BREAKIFFAILED(D3DCreateBlob(ibByteSize, &model.indexBufferCPU));
		//memcpy(model.indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
		
		//first copy to upload buffer then transfer to the default buffer 
		//default buffer has the maximum bandwidth for GPU, but not allow for CPU access

		BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vbByteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(vertexUploader.GetAddressOf())));
		vertexUploader->SetName(L"vertexUploader");
		
		BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vbByteSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(model.vertexBufferGPU.GetAddressOf())));
		model.vertexBufferGPU->SetName(L"vertex_buffer");
		
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData =(const void*) vertices.data();
		subresourceData.RowPitch = vbByteSize;
		subresourceData.SlicePitch = vbByteSize;

		//auto commandList = Graphics::gCommandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT).get()->getCommandList();
		
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(model.vertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(commandList.Get(), model.vertexBufferGPU.Get(), vertexUploader.Get(), 0, 0, 1, &subresourceData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(model.vertexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

		
		//index data

		BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(ibByteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(indexUploader.GetAddressOf())));
		indexUploader->SetName(L"indexUploader");
		BREAKIFFAILED(Graphics::gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(ibByteSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(model.indexBufferGPU.GetAddressOf())));
		model.indexBufferGPU->SetName(L"index_buffer");

		D3D12_SUBRESOURCE_DATA subresourceData_index = {};
		subresourceData_index.pData = (const void*)indices.data();
		subresourceData_index.RowPitch = ibByteSize;
		subresourceData_index.SlicePitch = ibByteSize;

		
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(model.indexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(commandList.Get(), model.indexBufferGPU.Get(), vertexUploader.Get(), 0, 0, 1, &subresourceData_index);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(model.indexBufferGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

		
		
		prim.iformat = DXGI_FORMAT_R32_UINT;
		prim.ibOffset = 0;
		prim.indexBufferByteSize = ibByteSize;
		prim.indexCount = indices.size();
		prim.vbOffset = 0;
		prim.vertexBufferByteSize = vbByteSize;

		

		model.meshes.push_back(Mesh{});
		model.meshes[0].primitives.push_back(prim);
		
		auto node = Node{};
		XMStoreFloat4x4(&node.matrix, Math::IdentityMatrix());
		node.mesh = 0;
		node.ibOffset = prim.ibOffset;
		node.indexCount = prim.indexCount;
		node.vbOffset = prim.vbOffset;

		model.nodes.push_back(node);
		model.numNodes = 1;
		model.numMeshes = 1; 


		model.vertexBufferView.BufferLocation = model.vertexBufferGPU->GetGPUVirtualAddress();
		model.vertexBufferView.StrideInBytes = sizeof(Vertex);
		model.vertexBufferView.SizeInBytes = prim.vertexBufferByteSize;

		model.indexBufferView.BufferLocation = model.indexBufferGPU->GetGPUVirtualAddress();
		model.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		model.indexBufferView.SizeInBytes = prim.indexBufferByteSize;

		/*auto testRItem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&testRItem->world, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));
		DirectX::XMStoreFloat4x4(&testRItem->texTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
		
		testRItem->model = &model;
		testRItem->meshIndex = 0;
		testRItem->primList.push_back(0);*/


		//create texture 


		

		return 1;
	}
}