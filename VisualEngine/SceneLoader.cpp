#include "pch.h"
#include "SceneLoader.h"
#include "Graphics.h"
#include "tiny_gltf.h"
#include "Renderer.h"
#include <iterator>
#include <vector>
#include "DDSTextureLoader.h"
#include "ResourceUploadBatch.h"

using Microsoft::WRL::ComPtr;

namespace Scene {

	ComPtr<ID3D12Resource> vertexPosUploader;
	ComPtr<ID3D12Resource> vertexNormalUploader;
	ComPtr<ID3D12Resource> vertexTexCordUploader;
	ComPtr<ID3D12Resource> vertexTangentUploader;
	ComPtr<ID3D12Resource> vertexColorUploader;

	ComPtr<ID3D12Resource> indexUploader;
	
	void SolveMeshs(tinygltf::Model& tinyModel, int meshIndex , Scene::Model& model, Scene::Node& node, DirectX::XMMATRIX& localToObject) {
		
		Mesh& mesh = model.meshes[meshIndex] = Mesh{};
		tinygltf::Mesh& tinyMesh = tinyModel.meshes[meshIndex];

		//buffer views 
		for (UINT i = 0; i < tinyModel.bufferViews.size(); i++) {
			const tinygltf::BufferView& tinyBufferView = tinyModel.bufferViews[i];
			if (tinyBufferView.target == 0) {
				continue;
			}

			const tinygltf::Buffer& tinyBuffer = tinyModel.buffers[tinyBufferView.buffer];
			size_t size = tinyBuffer.data.size();
			size_t offset = tinyBufferView.byteOffset;

			Utils::Print(("\ntarget:"+ tinyBufferView.name+"size:" + std::to_string(size) + ", offset:" + std::to_string(offset) + "\n").c_str());
		}

		mesh.primitives.resize(tinyMesh.primitives.size());
		for (UINT i = 0; i < tinyMesh.primitives.size(); i++) {
			tinygltf::Primitive& tinyPrimitive = tinyMesh.primitives[i];
			Primitive& primitive = mesh.primitives[i];
			{	
				//Index buffer
				tinygltf::Accessor indexAccessor = tinyModel.accessors[tinyPrimitive.indices];
				tinygltf::BufferView& indexBufferview =  tinyModel.bufferViews[indexAccessor.bufferView];

				ASSERT(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
				ASSERT(indexAccessor.type == TINYGLTF_TYPE_SCALAR)

				//concadinate primitives index buffers into one buffer
				UINT ibOffset = model.indexBufferCPU.size();
				auto& indexDataBuffer = tinyModel.buffers[indexBufferview.buffer].data;
				Utils::Print(std::to_string(indexDataBuffer.size()).c_str());
				auto itBegin = std::next(tinyModel.buffers[indexBufferview.buffer].data.begin(), indexAccessor.byteOffset + indexBufferview.byteOffset);
				auto itEnd = std::next(tinyModel.buffers[indexBufferview.buffer].data.begin(), indexAccessor.byteOffset + indexBufferview.byteOffset + indexBufferview.byteLength);
				model.indexBufferCPU.insert(model.indexBufferCPU.end(), 
											make_move_iterator(itBegin),
											make_move_iterator(itEnd));
				//TO EDIT:
				//primitive = indexBufferview.buffer;
				primitive.iformat = DXGI_FORMAT_R16_UINT;
				primitive.ibOffset = ibOffset;
				primitive.indexBufferByteSize = indexBufferview.byteLength;
				primitive.indexCount = (UINT)indexAccessor.count;

				model.indexBufferByteSize += indexBufferview.byteLength;
				node.indexCount = (UINT)indexAccessor.count;

				//prim.vbOffset = 0;
				//prim.vertexBufferByteSize = vertexBufferSize;
				//prim.indexBufferByteSize = indexBufferSize;
				//prim.vbOffset = 0;
				//prim.vertexBufferByteSize = vertexBufferSize;
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


				if (attribute.first.compare("POSITION") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC3 );
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					
					UINT vbOffset = model.vertexPosBufferCPU.size();
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset + vaBufferview.byteLength);
					model.vertexPosBufferCPU.insert(model.vertexPosBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));

					primitive.vertexCount = accessor.count;

					//primitive.vbOffset = vbOffset;
					//primitive.vertexBufferByteSize = vaBufferview.byteLength;
					//primitive.vertexCount = (UINT)accessor.count;

					model.vertexPosBufferByteSize = vaBufferview.byteLength;
					 
				} 

				else if (attribute.first.compare("NORMAL") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC3);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					UINT vbOffset = model.vertexNormalBufferCPU.size();
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset + vaBufferview.byteLength);
					model.vertexNormalBufferCPU.insert(model.vertexNormalBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));

					model.vertexNormalBufferByteSize = vaBufferview.byteLength;
				} 
				else if (attribute.first.compare("TEXCOORD_0") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC2);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					UINT vbOffset = model.vertexTexCordBufferCPU.size();
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset + vaBufferview.byteLength);
					model.vertexTexCordBufferCPU.insert(model.vertexTexCordBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));
					model.vertexTexCordBufferByteSize = vaBufferview.byteLength;

				}
				else if (attribute.first.compare("TANGENT") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC4);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)\
					UINT vbOffset = model.vertexTangentBufferCPU.size();
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), accessor.byteOffset + vaBufferview.byteOffset + vaBufferview.byteLength);
					model.vertexTangentBufferCPU.insert(model.vertexTangentBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));
					model.vertexTangentBufferByteSize = vaBufferview.byteLength;
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
		model.nodes[nodeIndex].numFrameDirty = 3;
		tinygltf::Node& tinyNode = tinyModel.nodes[nodeIndex];
		//validate current mesh
		
		if ((tinyNode.mesh >= 0) && (tinyNode.mesh < tinyModel.meshes.size())) {

			if (tinyNode.matrix.size() != 0) {
				localToObject = Math::VectorToMatrix(tinyNode.matrix) * localToObject;
			}
			else {
				DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(10,10,10); //Math::Scale(tinyNode.scale);

				DirectX::XMMATRIX translate = Math::Trans(tinyNode.translation);
				DirectX::XMMATRIX rotate = Math::QuaternionToMatrix(tinyNode.rotation);
				
				DirectX::XMMATRIX m = translate * rotate * scale;
				localToObject = scale  * translate; //* rotate * translate;
				
				
			}
			DirectX::XMStoreFloat4x4(&model.nodes[nodeIndex].matrix, localToObject);
			SolveMeshs(tinyModel, tinyNode.mesh, model, model.nodes[nodeIndex], localToObject);
		}

		for (UINT i = 0; i < tinyNode.children.size(); i++) {
			ASSERT((tinyNode.children[i] >= 0) && (tinyNode.children[i] < tinyModel.nodes.size()));
			SolveNodes(tinyModel, tinyNode.children[i], model, localToObject);
		}

		

	}

	void SolveMaterials(tinygltf::Model& tinyModel, Scene::Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {
		auto& materials  = tinyModel.materials;

		for (UINT i = 0; i < materials.size(); i++) {
			auto& tinyMat = materials[i];
			auto mat = std::make_unique<Material>();
			mat->name = tinyMat.name;

			//material properties 
			auto& baseColorFactor =  tinyMat.pbrMetallicRoughness.baseColorFactor;
			mat->diffuse = { (float)baseColorFactor[0],  (float)baseColorFactor[1],  (float)baseColorFactor[2] };
			mat->emissive= { (float)tinyMat.emissiveFactor[0],  (float)tinyMat.emissiveFactor[1],  (float)tinyMat.emissiveFactor[2] };
			mat->metalness = tinyMat.pbrMetallicRoughness.metallicFactor;
			mat->roughness = tinyMat.pbrMetallicRoughness.roughnessFactor;

			//extract texture data  
			//diffuse map  
			
			auto diffuseTex = std::make_unique<Texture>();
			int diffuseIndex = tinyMat.pbrMetallicRoughness.baseColorTexture.index;

			tinygltf::Texture& tinyDiffuseTex = tinyModel.textures[diffuseIndex];
			tinygltf::Image& tinyDiffuseTexImg = tinyModel.images[tinyDiffuseTex.source]; 
			diffuseTex->name = tinyDiffuseTex.name;
			mat->texDiffuseMap = tinyDiffuseTex.name;
			diffuseTex->uri = tinyDiffuseTexImg.uri;
			diffuseTex->width = tinyDiffuseTexImg.width;
			diffuseTex->height = tinyDiffuseTexImg.height;
			diffuseTex->component = tinyDiffuseTexImg.component;
			diffuseTex->bits = tinyDiffuseTexImg.bits;

			
			D3D12_RESOURCE_DESC txtDesc = {};
			txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
			if(diffuseTex->bits==8 && diffuseTex->component == 4)
				txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			txtDesc.Width = tinyDiffuseTexImg.width;
			txtDesc.Height = tinyDiffuseTexImg.height;
			txtDesc.SampleDesc.Count = 1;
			txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
			Graphics::gDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&txtDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(diffuseTex->textureResource.ReleaseAndGetAddressOf()));
			
			std::unique_ptr<uint8_t[]> decodedData;
			D3D12_SUBRESOURCE_DATA subresource = {};
			subresource.pData = tinyDiffuseTexImg.image.data();
			subresource.RowPitch = static_cast<LONG_PTR>(tinyDiffuseTexImg.width) * diffuseTex->component;
			subresource.SlicePitch = subresource.RowPitch * tinyDiffuseTexImg.height;
			/*
			
			std::unique_ptr<uint8_t[]> wicData;
			BREAKIFFAILED(
				DirectX::LoadWICTextureFromFile(
					Graphics::gDevice.Get(),
					L"Models\\Avocado\\glTF\\Avocado_baseColor.png",
					diffuseTex->textureResource.ReleaseAndGetAddressOf(),
					wicData,
					subresource

				));
			
			*/

			UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), diffuseTex->textureResource, diffuseTex->textureUploader, subresource);
			diffuseTex->textureUploader->SetName(L"diffuseTexUploader");
			diffuseTex->textureResource->SetName(L"diffuseTexDefault");
			model.textures[diffuseTex->name] = std::move(diffuseTex);

			/*
			//roughnessMetallic
			auto roughnessMetallicTex = std::make_unique<Texture>();
			int roughnessMetallicIndex = tinyMat.pbrMetallicRoughness.metallicRoughnessTexture.index;

			tinygltf::Texture& tinyRoughnessMetallicTex = tinyModel.textures[roughnessMetallicIndex];
			tinygltf::Image& tinyRoughnessMetallicImg = tinyModel.images[tinyRoughnessMetallicTex.source];
			roughnessMetallicTex->name = tinyRoughnessMetallicTex.name;
			mat->texroughnessMetallicMap = tinyRoughnessMetallicTex.name;
			roughnessMetallicTex->uri = tinyRoughnessMetallicImg.uri;
			roughnessMetallicTex->width = tinyRoughnessMetallicImg.width;
			roughnessMetallicTex->height = tinyRoughnessMetallicImg.height;
			roughnessMetallicTex->component = tinyRoughnessMetallicImg.component;
			roughnessMetallicTex->bits = tinyRoughnessMetallicImg.bits;

			BREAKIFFAILED(
				DirectX::LoadWICTextureFromMemory(
					Graphics::gDevice.Get(),
					tinyRoughnessMetallicImg.image.data(),
					tinyRoughnessMetallicImg.image.size(),
					roughnessMetallicTex->textureResource.ReleaseAndGetAddressOf(),
					roughnessMetallicTex->decodedData,
					subresource));
			UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), roughnessMetallicTex->textureResource, roughnessMetallicTex->textureUploader, subresource);
			model.textures[roughnessMetallicTex->name] = std::move(roughnessMetallicTex);


			//normal 
			auto normalTex = std::make_unique<Texture>();
			int normalIndex = tinyMat.normalTexture.index;

			tinygltf::Texture& tinyNormalTex = tinyModel.textures[normalIndex];
			tinygltf::Image& tinyNormalTexImg = tinyModel.images[tinyNormalTex.source];
			normalTex->name = tinyNormalTex.name;
			mat->texNormalMap = tinyNormalTex.name;
			normalTex->uri = tinyNormalTexImg.uri;
			normalTex->width = tinyNormalTexImg.width;
			normalTex->height = tinyNormalTexImg.height;
			normalTex->component = tinyNormalTexImg.component;
			normalTex->bits = tinyNormalTexImg.bits;

			

			BREAKIFFAILED(
				DirectX::LoadWICTextureFromMemory(
					Graphics::gDevice.Get(),
					tinyNormalTexImg.image.data(),
					tinyNormalTexImg.image.size(),
					normalTex->textureResource.ReleaseAndGetAddressOf(),
					normalTex->decodedData,
					subresource));
			UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), normalTex->textureResource, normalTex->textureUploader, subresource);
			model.textures[normalTex->name] = std::move(normalTex);
			*/

			model.materials[mat->name] = std::move(mat);
		}

		
	}

	int translate(tinygltf::Model& tinyModel, Scene::Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {
		const tinygltf::Scene& scene = tinyModel.scenes[tinyModel.defaultScene];
		//model.buffers.resize(tinyModel.buffers.size());
		model.meshes.resize(tinyModel.meshes.size());
		model.nodes.resize(tinyModel.nodes.size());

		model.numNodes = tinyModel.nodes.size();
		model.numMeshes = tinyModel.meshes.size();
		model.numMaterials = tinyModel.materials.size();
		model.numTextures = tinyModel.textures.size();

		//DirectX::XMLoadFloat4x4(model.);

		//TO EDIT: NO NEED TO MOVE OVER ALL BUFFERS, AS THEY WILL BE CLASSIFIED AND TRANSFERED TO GPU RESOURCE
		//move over all buffers 
		//for (UINT i = 0; i < tinyModel.buffers.size(); i++) {
		//	model.buffers[i] = std::move(tinyModel.buffers[i].data);
		//}

		//iterate over all nodes 
		for (UINT i = 0; i < scene.nodes.size(); i++) {
			SolveNodes(tinyModel, scene.nodes[i], model, Math::IdentityMatrix());
		}

		SolveMaterials(tinyModel, model, commandList);

		
		return 1;
	}



	int LoadScene(std::wstring filename, Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {
		
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
		translate(tinyModel, model, commandList);

		model.indexBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.indexBufferCPU.data(), model.indexBufferByteSize, indexUploader);
		model.vertexPosBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.vertexPosBufferCPU.data(), model.vertexPosBufferByteSize, vertexPosUploader);

		model.vertexPosBufferView = {};
		model.vertexPosBufferView.BufferLocation = model.vertexPosBufferGPU->GetGPUVirtualAddress();
		model.vertexPosBufferView.StrideInBytes = sizeof(VertexPos);
		model.vertexPosBufferView.SizeInBytes = model.vertexPosBufferByteSize;

		model.indexBufferView = {};
		model.indexBufferView.BufferLocation = model.indexBufferGPU->GetGPUVirtualAddress();
		model.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		model.indexBufferView.SizeInBytes = model.indexBufferByteSize;


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


		std::vector<VertexPos> verticesPos(vcount);
		std::vector<VertexColor> verticesColor(vcount);
		for (UINT i = 0; i < vcount; ++i) {
			fin >> verticesPos[i].position.x >> verticesPos[i].position.y >> verticesPos[i].position.z;
			fin >> verticesColor[i].color.x >> verticesColor[i].color.y >> verticesColor[i].color.z;
		}

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		std::vector<std::uint16_t> indices(3 * tcount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}
		fin.close();

		const UINT vertexPosBufferSize = sizeof(VertexPos) * (UINT)verticesPos.size();
		const UINT vertexColorBufferSize = sizeof(VertexColor) * (UINT)verticesColor.size();
		const UINT indexBufferSize = sizeof(uint16_t) * (UINT)indices.size();

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.

		model.vertexPosBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), verticesPos.data(), vertexPosBufferSize, vertexPosUploader);
		model.vertexColorBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), verticesColor.data(), vertexColorBufferSize, vertexColorUploader);
				

		model.indexBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), indices.data(), indexBufferSize, indexUploader);
		
		
		// Copy the triangle data to the vertex buffer.
		//UINT8* pVertexDataBegin;
		//CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		//BREAKIFFAILED(vertexUploader->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		//memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		//vertexUploader->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		model.vertexPosBufferView = {};
		model.vertexPosBufferView.BufferLocation = model.vertexPosBufferGPU->GetGPUVirtualAddress();
		model.vertexPosBufferView.StrideInBytes = sizeof(VertexPos);
		model.vertexPosBufferView.SizeInBytes = vertexPosBufferSize;

		model.vertexColorBufferView = {};
		model.vertexColorBufferView.BufferLocation = model.vertexColorBufferGPU->GetGPUVirtualAddress();
		model.vertexColorBufferView.StrideInBytes = sizeof(VertexColor);
		model.vertexColorBufferView.SizeInBytes = vertexColorBufferSize;

		model.indexBufferView = {};
		model.indexBufferView.BufferLocation = model.indexBufferGPU->GetGPUVirtualAddress();
		model.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		model.indexBufferView.SizeInBytes = indexBufferSize;

		auto prim = Primitive{};
		prim.iformat = DXGI_FORMAT_R16_UINT;
		prim.ibOffset = 0;
		prim.indexBufferByteSize = indexBufferSize;
		prim.indexCount = (UINT)indices.size();
		prim.vbOffset = 0;
		//prim.vertexBufferByteSize = vertexBufferSize;
		//prim.vertexCount = (UINT)vertices.size();

		model.meshes.push_back(Mesh{});
		model.meshes[0].primitives.push_back(prim);

		auto node = Node{};
		auto matrix = Math::IdentityMatrix();
		
		XMStoreFloat4x4(&node.matrix, Math::IdentityMatrix());
		node.mesh = 0;
		node.ibOffset = prim.ibOffset;
		node.indexCount = prim.indexCount;
		node.vbOffset = prim.vbOffset;
		node.numFrameDirty = 3;

		model.nodes.push_back(node);
		model.numNodes = 1;
		model.numMeshes = 1;


		//create testing material  
		auto tempMat = std::make_unique<Material>();
		tempMat->name = "brick";
		tempMat->MatCBIndex = 0;
		tempMat->diffuse = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
		tempMat->roughness = 0.2f;
		tempMat->metalness = 0.0f;
		model.materials[tempMat->name] = (std::move(tempMat));

		return 1;
	}

	void UploadToDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
		D3D12_SUBRESOURCE_DATA& subResourceData) {

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(defaultBuffer.Get(), 0, 1);
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		BREAKIFFAILED(
		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

		
		UpdateSubresources(cmdList, defaultBuffer.Get(), uploadBuffer.Get(),
			0, 0, 1, &subResourceData);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}


	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
	{
		ComPtr<ID3D12Resource> defaultBuffer;

		// Create the actual default buffer resource.
		BREAKIFFAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

		// In order to copy CPU memory data into our default buffer, we need to create
		// an intermediate upload heap. 
		BREAKIFFAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = initData;
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
		// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
		// the intermediate upload heap data will be copied to mBuffer.
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

		// Note: uploadBuffer has to be kept alive after the above function calls because
		// the command list has not been executed yet that performs the actual copy.
		// The caller can Release the uploadBuffer after it knows the copy has been executed.


		return defaultBuffer;
	}
	

}
