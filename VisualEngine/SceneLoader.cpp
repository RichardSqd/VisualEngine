#include "pch.h"
#include "SceneLoader.h"
#include "Graphics.h"
#include "tiny_gltf.h"
#include "Renderer.h"
#include <iterator>
#include <vector>
#include "ResourceUploadBatch.h"
#include "ShaderLightingData.h"
#include "pix3.h"
#include "Cubemap.h"
#include "DirectXTex.h"

using Microsoft::WRL::ComPtr;

namespace Scene {

	ComPtr<ID3D12Resource> vertexPosUploader;
	ComPtr<ID3D12Resource> vertexNormalUploader;
	ComPtr<ID3D12Resource> vertexTexCordUploader;
	ComPtr<ID3D12Resource> vertexTangentUploader;
	ComPtr<ID3D12Resource> vertexColorUploader;

	ComPtr<ID3D12Resource> indexUploader;

	void CreateCubeMapGeo(ComPtr<ID3D12GraphicsCommandList> commandList);

	void LoadIBLImage(ComPtr<ID3D12GraphicsCommandList> commandList, Scene::Model& model) {
		stbi_set_flip_vertically_on_load(true);
		int width = 0;
		int height = 0;

		DirectX::TexMetadata metadata {};
		DirectX::ScratchImage scratchImage{};
		DirectX::LoadFromHDRFile(Config::iblImagePath.c_str(), &metadata, scratchImage);

		if (metadata.mipLevels>0) {
			auto iblTex = std::make_unique<Texture>();
			iblTex->name = "IBL_Texture";
			iblTex->uri = Utils::to_byte_str(Config::iblImagePath) ;
			iblTex->width = metadata.width;
			iblTex->height = metadata.height;

			D3D12_SUBRESOURCE_DATA subresource{};
			subresource.pData = scratchImage.GetImages()->pixels;
			subresource.RowPitch = scratchImage.GetImages()->rowPitch;
			subresource.SlicePitch = scratchImage.GetImages()->slicePitch;

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = metadata.mipLevels;
			textureDesc.Format = metadata.format;
			textureDesc.Width = metadata.width;
			textureDesc.Height = metadata.height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			Graphics::gDevice.Get()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(iblTex->textureResource.GetAddressOf()));

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(iblTex->textureResource.Get(), 0, 1);

			Graphics::gDevice.Get()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(iblTex->textureUploader.GetAddressOf()));

			UpdateSubresources(commandList.Get(), iblTex->textureResource.Get(), iblTex->textureUploader.Get(),
				0, 0, 1, &subresource);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(iblTex->textureResource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			//UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), iblTex->textureResource, iblTex->textureUploader, subresource);
			//auto diffuseName = std::wstring(diffuseTex->name.begin(), diffuseTex->name.end());
			iblTex->textureUploader->SetName(L"IBL Uploader");
			iblTex->textureResource->SetName(L"IBL Default");
			model.textures[iblTex->name] = std::move(iblTex);
			
		}
	}
	
	void SolveMeshs(tinygltf::Model& tinyModel, int meshIndex , Scene::Model& model, Scene::Node& node, DirectX::XMMATRIX& localToObject) {
		

		model.meshes[meshIndex] = Mesh{};
		Mesh& mesh = model.meshes[meshIndex];
		tinygltf::Mesh& tinyMesh = tinyModel.meshes[meshIndex];

		//buffer views 
		

		mesh.primitives.resize(tinyMesh.primitives.size());
		model.numPrimitives += (UINT)tinyMesh.primitives.size();
		for (UINT i = 0; i < tinyMesh.primitives.size(); i++) {
			tinygltf::Primitive& tinyPrimitive = tinyMesh.primitives[i];
			Primitive& primitive = mesh.primitives[i];

			//set material 
			primitive.matIndex = tinyPrimitive.material + 1 ;
			

			if (tinyPrimitive.indices >= 0) 
			{
				//Index buffer
				primitive.hasIndices = true;
				tinygltf::Accessor indexAccessor = tinyModel.accessors[tinyPrimitive.indices];
				tinygltf::BufferView& indexBufferview =  tinyModel.bufferViews[indexAccessor.bufferView];
				ASSERT(indexAccessor.type == TINYGLTF_TYPE_SCALAR)

				//concadinate primitives index buffers into one buffer
				UINT ibOffset = model.indexBufferCPU.size();
				auto& indexDataBuffer = tinyModel.buffers[indexBufferview.buffer].data;
				//Utils::Print(std::to_string(indexDataBuffer.size()).c_str());
				
				//TO EDIT:
				//primitive = indexBufferview.buffer;
				auto stride = 0 ; 
				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					primitive.iformat = DXGI_FORMAT_R16_UINT;
					stride = 2;
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					primitive.iformat = DXGI_FORMAT_R32_UINT;
					stride = 4;
				}
				else {
					__debugbreak();
				}

				auto start = indexBufferview.byteOffset + indexAccessor.byteOffset;
				auto end = start + indexAccessor.count * stride;
				auto itBegin = std::next(tinyModel.buffers[indexBufferview.buffer].data.begin(), start);
				auto itEnd = std::next(tinyModel.buffers[indexBufferview.buffer].data.begin(), end);
				model.indexBufferCPU.insert(model.indexBufferCPU.end(),
					make_move_iterator(itBegin),
					make_move_iterator(itEnd));
				
				primitive.ibOffset = ibOffset;
				primitive.indexBufferByteSize = end - start;
				primitive.indexCount = (UINT)indexAccessor.count;
					


				model.indexBufferByteSize += end - start;

			}
			else {
				primitive.hasIndices = false;
			}


			for (auto& attribute : tinyPrimitive.attributes) {
				tinygltf::Accessor accessor = tinyModel.accessors[attribute.second];
				tinygltf::BufferView vaBufferview = tinyModel.bufferViews[accessor.bufferView];

				//int byteStride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);
				//size_t byteOffset = accessor.byteOffset;


				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}


				if (attribute.first.compare("POSITION") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC3 );
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)

					auto stride = 0;
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3) {
						stride = 12;
					}


					UINT vbOffset = (UINT)model.vertexPosBufferCPU.size();
					auto start = accessor.byteOffset + vaBufferview.byteOffset;
					auto end = start + accessor.count * stride;
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), start);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), end);
					model.vertexPosBufferCPU.insert(model.vertexPosBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));

					primitive.vertexCount = (UINT)accessor.count;

					primitive.vbPosOffset = vbOffset;
					primitive.vertexBufferPosByteSize = end - start;

					model.vertexPosBufferByteSize += end - start;

				} 

				else if (attribute.first.compare("NORMAL") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC3);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					auto stride = 0;
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3) {
						stride = 12;
					}

					UINT vbOffset = model.vertexNormalBufferCPU.size();
					auto start = accessor.byteOffset + vaBufferview.byteOffset;
					auto end = start + accessor.count * stride;
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), start);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), end);
					model.vertexNormalBufferCPU.insert(model.vertexNormalBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));

					primitive.vbNormalOffset = vbOffset;
					primitive.vbNormalBufferByteSize = end - start;

					model.vertexNormalBufferByteSize += end - start;

				} 


				else if (attribute.first.compare("TEXCOORD_0") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC2);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					auto stride = 0;
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2) {
						stride = 8;
					}
					UINT vbOffset = model.vertexTexCordBufferCPU.size();
					auto start = accessor.byteOffset + vaBufferview.byteOffset;
					auto end = start + accessor.count * stride;
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), start);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), end);
					model.vertexTexCordBufferCPU.insert(model.vertexTexCordBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));
					primitive.vbTexOffset = vbOffset;
					primitive.vertexBufferTexCordByteSize = end - start;

					model.vertexTexCordBufferByteSize += end - start;
					//model.vertexTexCordBufferByteSize += vaBufferview.byteLength;

				}
				else if (attribute.first.compare("TANGENT") == 0) {
					ASSERT(accessor.type == TINYGLTF_TYPE_VEC4);
					ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					auto stride = 0;
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4) {
						stride = 16;
					}
					UINT vbOffset = model.vertexTangentBufferCPU.size();
					auto start = accessor.byteOffset + vaBufferview.byteOffset;
					auto end = start + accessor.count * stride;
					auto vbBegin = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), start);
					auto vbEnd = std::next(tinyModel.buffers[vaBufferview.buffer].data.begin(), end);
					model.vertexTangentBufferCPU.insert(model.vertexTangentBufferCPU.end(),
						make_move_iterator(vbBegin),
						make_move_iterator(vbEnd));
					//model.vertexTangentBufferByteSize += vaBufferview.byteLength;

					primitive.vbTangentOffset = vbOffset;
					primitive.vbTangentBufferByteSize = end - start;
					model.vertexTangentBufferByteSize += end - start;
				}
				//Utils::Print(attribute.first.c_str());
				

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
	void SolveNodes(tinygltf::Model& tinyModel, int nodeIndex, Scene::Model& model, DirectX::XMMATRIX& localToObject, std::vector<bool> visited) {
		
		
		model.nodes[nodeIndex] = Node{};
		model.nodes[nodeIndex].cbdHeapIndexByFrames.resize(Graphics::gNumFrameResources);
		model.nodes[nodeIndex].numFrameDirty = Graphics::gNumFrameResources;
		tinygltf::Node& tinyNode = tinyModel.nodes[nodeIndex];
		model.nodes[nodeIndex].mesh = tinyNode.mesh;
		
		if (tinyNode.matrix.size() != 0) {
			localToObject = localToObject * Math::VectorToMatrix(tinyNode.matrix);
		}
		else {
			DirectX::XMMATRIX scale;
			DirectX::XMMATRIX rotate;
			DirectX::XMMATRIX translate;
			if (tinyNode.scale.size() == 3) {
				scale = DirectX::XMMatrixScaling((float)tinyNode.scale[0] * 1.0, (float)tinyNode.scale[1] * 1.0, (float)tinyNode.scale[2] * 1.0);
			}
			else {
				scale = DirectX::XMMatrixScaling(1, 1, 1);
			}
			if (tinyNode.rotation.size() == 4) {
				DirectX::XMVECTOR rv = DirectX::XMVectorSet(tinyNode.rotation[0], tinyNode.rotation[1], tinyNode.rotation[2], tinyNode.rotation[3]);
				rotate = DirectX::XMMatrixRotationQuaternion(rv);
			}
			else {
				rotate = DirectX::XMMatrixIdentity();
			}

			if (tinyNode.translation.size() == 3) {
				translate = DirectX::XMMatrixTranslation((float)tinyNode.translation[0], (float)tinyNode.translation[1], (float)tinyNode.translation[2]);

			}
			else {
				translate = DirectX::XMMatrixTranslation(0, 0, 0);

			}

			//DirectX::XMMATRIX rotate = Math::QuaternionToMatrix(tinyNode.rotation);
			localToObject = (scale * rotate * translate) ;

		}

		DirectX::XMStoreFloat4x4(&model.nodes[nodeIndex].toWorldmatrix, localToObject);
		

		//validate current mesh
		if ((tinyNode.mesh >= 0) && (tinyNode.mesh < tinyModel.meshes.size()) ){
			
			if (model.meshes[tinyNode.mesh].primitives.size() == 0) {
				SolveMeshs(tinyModel, tinyNode.mesh, model, model.nodes[nodeIndex], localToObject);
			}
		}
		

		for (UINT i = 0; i < tinyNode.children.size(); i++) {
			auto nodeIndex = tinyNode.children[i];
			ASSERT((nodeIndex >= 0) && (nodeIndex < tinyModel.nodes.size()));
			if (!visited[nodeIndex]) {
				visited[nodeIndex] = true;
				SolveNodes(tinyModel, nodeIndex, model, localToObject, visited);
			}
			
			
		}

		

	}

	void SolveMaterials(tinygltf::Model& tinyModel, Scene::Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {
		auto& materials  = tinyModel.materials;

		//create the default material at index 0
		{
			auto mat = std::make_unique<Material>();
			mat->name = "default_material";
			mat->diffuse = { 0.0, 0.0, 1.0, 1.0 };
			mat->emissive = { 0.0, 0.0, 0.0 };
			mat->metalness = 1.0f;
			mat->roughness = 0.2f;
			mat->hasDiffuseTexture = 0;
			mat->texDiffuseMap = "";
			mat->hasMetallicRoughnessTexture = 0;
			mat->texroughnessMetallicMap = "";
			mat->hasNormalTexture = 0;
			mat->texNormalMap = "";
			mat->hasOcclusionTexture = 0;
			mat->texOcclusionMap = "";
			mat->hasEmissiveTexture = 0;
			mat->texEmisiveMap = "";
			
			model.materials[0] = std::move(mat);
		}

		for (UINT i = 0; i < materials.size(); i++) {
			auto& tinyMat = materials[i];
			auto mat = std::make_unique<Material>();
			mat->name = tinyMat.name;

			//material properties 
			auto& baseColorFactor =  tinyMat.pbrMetallicRoughness.baseColorFactor;
			mat->diffuse = { (float)baseColorFactor[0],  (float)baseColorFactor[1],  (float)baseColorFactor[2], (float)baseColorFactor[3] };
			mat->emissive= { (float)tinyMat.emissiveFactor[0],  (float)tinyMat.emissiveFactor[1],  (float)tinyMat.emissiveFactor[2] };
			mat->metalness = (float)tinyMat.pbrMetallicRoughness.metallicFactor;
			mat->roughness = (float)tinyMat.pbrMetallicRoughness.roughnessFactor;

			//extract texture data  
			//diffuse map  
			
			D3D12_SUBRESOURCE_DATA subresource = {};
			std::wstring filepath;

			int diffuseIndex = tinyMat.pbrMetallicRoughness.baseColorTexture.index;
			if (diffuseIndex >= 0) {
				auto diffuseTex = std::make_unique<Texture>();
				tinygltf::Texture& tinyDiffuseTex = tinyModel.textures[diffuseIndex];
				tinygltf::Image& tinyDiffuseTexImg = tinyModel.images[tinyDiffuseTex.source];
				auto name = tinyMat.name.size() == 0 ? "mat" + std::to_string(i) : tinyMat.name;
				diffuseTex->name = name + "_diffuse";
				mat->texDiffuseMap = name + "_diffuse";
				diffuseTex->uri = tinyDiffuseTexImg.uri;
				diffuseTex->width = tinyDiffuseTexImg.width;
				diffuseTex->height = tinyDiffuseTexImg.height;
				diffuseTex->component = tinyDiffuseTexImg.component;
				diffuseTex->bits = tinyDiffuseTexImg.bits;

				/*
				D3D12_RESOURCE_DESC txtDesc = {};
				txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
				if(diffuseTex->bits==8 && diffuseTex->component == 4)
					txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				txtDesc.Width = tinyDiffuseTexImg.width;
				txtDesc.Height = tinyDiffuseTexImg.height;
				txtDesc.SampleDesc.Count = 1;
				txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				*/
				/*
				CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
				Graphics::gDevice->CreateCommittedResource(
						&heapProps,
						D3D12_HEAP_FLAG_NONE,
						&txtDesc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						IID_PPV_ARGS(diffuseTex->textureResource.ReleaseAndGetAddressOf()));
				*/
				//std::unique_ptr<uint8_t[]> decodedData;
				
				//subresource.pData = tinyDiffuseTexImg.image.data();
				//subresource.RowPitch = static_cast<LONG_PTR>(tinyDiffuseTexImg.width) * diffuseTex->component;
				//subresource.SlicePitch = subresource.RowPitch * tinyDiffuseTexImg.height;

				filepath = Config::gltfFileDirectory + L"\\" + Utils::to_wide_str(diffuseTex->uri);
				std::unique_ptr<uint8_t[]> wicDiffuseData;
				BREAKIFFAILED(
					DirectX::LoadWICTextureFromFile(
						Graphics::gDevice.Get(),
						filepath.c_str(),
						diffuseTex->textureResource.ReleaseAndGetAddressOf(),
						wicDiffuseData,
						subresource

					));



				UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), diffuseTex->textureResource, diffuseTex->textureUploader, subresource);
				auto diffuseName = std::wstring(diffuseTex->name.begin(), diffuseTex->name.end());
				diffuseTex->textureUploader->SetName((diffuseName + L"Uploader").c_str());
				diffuseTex->textureResource->SetName((diffuseName + L"Default").c_str());
				model.textures[diffuseTex->name] = std::move(diffuseTex);
				mat->hasDiffuseTexture = 1;
			}
			else {
				mat->hasDiffuseTexture = 0;
			}
			

			
			//roughnessMetallic texture
			
			int roughnessMetallicIndex = tinyMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (roughnessMetallicIndex >= 0) {
				auto roughnessMetallicTex = std::make_unique<Texture>();
				tinygltf::Texture& tinyRoughnessMetallicTex = tinyModel.textures[roughnessMetallicIndex];
				tinygltf::Image& tinyRoughnessMetallicImg = tinyModel.images[tinyRoughnessMetallicTex.source];
				auto name = tinyMat.name.size() == 0 ? "mat" + std::to_string(i) : tinyMat.name;
				roughnessMetallicTex->name = name + "_roughnessMetallic";
				mat->texroughnessMetallicMap = name + "_roughnessMetallic";
				roughnessMetallicTex->uri = tinyRoughnessMetallicImg.uri;
				roughnessMetallicTex->width = tinyRoughnessMetallicImg.width;
				roughnessMetallicTex->height = tinyRoughnessMetallicImg.height;
				roughnessMetallicTex->component = tinyRoughnessMetallicImg.component;
				roughnessMetallicTex->bits = tinyRoughnessMetallicImg.bits;

				filepath = Config::gltfFileDirectory + L"\\" + Utils::to_wide_str(roughnessMetallicTex->uri);
				std::unique_ptr<uint8_t[]> wicRoughnessMetallicData;
				BREAKIFFAILED(
					DirectX::LoadWICTextureFromFile(
						Graphics::gDevice.Get(),
						filepath.c_str(),
						roughnessMetallicTex->textureResource.ReleaseAndGetAddressOf(),
						wicRoughnessMetallicData,
						subresource
					));

				UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), roughnessMetallicTex->textureResource, roughnessMetallicTex->textureUploader, subresource);
				auto roughnessMetallicName = std::wstring(roughnessMetallicTex->name.begin(), roughnessMetallicTex->name.end());
				roughnessMetallicTex->textureUploader->SetName((roughnessMetallicName +L"Uploader").c_str());
				roughnessMetallicTex->textureResource->SetName((roughnessMetallicName + L"Default").c_str());
				model.textures[roughnessMetallicTex->name] = std::move(roughnessMetallicTex);
				mat->hasMetallicRoughnessTexture = 1;
			}
			else {
				mat->hasMetallicRoughnessTexture = 0;
				mat->texroughnessMetallicMap = "";
			}
			

			//normal 
			
			int normalIndex = tinyMat.normalTexture.index;
			if (normalIndex >= 0) {
				auto normalTex = std::make_unique<Texture>();
				tinygltf::Texture& tinyNormalTex = tinyModel.textures[normalIndex];
				tinygltf::Image& tinyNormalTexImg = tinyModel.images[tinyNormalTex.source];
				auto name = tinyMat.name.size() == 0 ? "mat" + std::to_string(i) : tinyMat.name;
				normalTex->name = name + "_normal";
				mat->texNormalMap = name + "_normal";
				normalTex->uri = tinyNormalTexImg.uri;
				normalTex->width = tinyNormalTexImg.width;
				normalTex->height = tinyNormalTexImg.height;
				normalTex->component = tinyNormalTexImg.component;
				normalTex->bits = tinyNormalTexImg.bits;

				filepath = Config::gltfFileDirectory + L"\\" + Utils::to_wide_str(normalTex->uri);


				std::unique_ptr<uint8_t[]> wicNormalData;
				BREAKIFFAILED(
					DirectX::LoadWICTextureFromFile(
						Graphics::gDevice.Get(),
						filepath.c_str(),
						normalTex->textureResource.ReleaseAndGetAddressOf(),
						wicNormalData,
						subresource
					));

				UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), normalTex->textureResource, normalTex->textureUploader, subresource);
				auto normalTexName = std::wstring(normalTex->name.begin(), normalTex->name.end());
				normalTex->textureUploader->SetName((normalTexName + L"Uploader").c_str());
				normalTex->textureResource->SetName((normalTexName + L"Default").c_str());
				model.textures[normalTex->name] = std::move(normalTex);
				mat->hasNormalTexture = 1;
			}
			else {
				mat->hasNormalTexture = 0;
				mat->texNormalMap = "";
			}
			
			

			//occlusion texture
			
			int occlusionIndex = tinyMat.occlusionTexture.index;
			if (occlusionIndex >= 0) {
				auto occlusionTex = std::make_unique<Texture>();
				tinygltf::Texture& tinyOcclusionTex = tinyModel.textures[occlusionIndex];
				tinygltf::Image& tinyOcclusionTexImg = tinyModel.images[tinyOcclusionTex.source];
				auto name = tinyMat.name.size() == 0 ? "mat" + std::to_string(i) : tinyMat.name;
				occlusionTex->name = name + "_AO";
				mat->texOcclusionMap = name + "_AO";
				occlusionTex->uri = tinyOcclusionTexImg.uri;
				occlusionTex->width = tinyOcclusionTexImg.width;
				occlusionTex->height = tinyOcclusionTexImg.height;
				occlusionTex->component = tinyOcclusionTexImg.component;
				occlusionTex->bits = tinyOcclusionTexImg.bits;
				filepath = Config::gltfFileDirectory + L"\\" + Utils::to_wide_str(occlusionTex->uri);
				std::unique_ptr<uint8_t[]> wicAOData;
				BREAKIFFAILED(
					DirectX::LoadWICTextureFromFile(
						Graphics::gDevice.Get(),
						filepath.c_str(),
						occlusionTex->textureResource.ReleaseAndGetAddressOf(),
						wicAOData,
						subresource
					));

				UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), occlusionTex->textureResource, occlusionTex->textureUploader, subresource);
				auto occlusionTexName = std::wstring(occlusionTex->name.begin(), occlusionTex->name.end());
				occlusionTex->textureUploader->SetName((occlusionTexName + L"Uploader").c_str());
				occlusionTex->textureResource->SetName((occlusionTexName + L"Default").c_str());
				model.textures[occlusionTex->name] = std::move(occlusionTex);
				//occlusionTex->name = 
				mat->hasOcclusionTexture = 1;
			}
			else {
				mat->hasOcclusionTexture = 0;
				mat->texOcclusionMap = "";
			}

			//emmisive texture 
			int emmisiveIndex = tinyMat.emissiveTexture.index;
			if (emmisiveIndex >= 0) {
				auto emmisiveTex = std::make_unique<Texture>();
				tinygltf::Texture& tinyEmmisiveTex = tinyModel.textures[emmisiveIndex];
				tinygltf::Image& tinyEmmisiveTexImg = tinyModel.images[tinyEmmisiveTex.source];
				auto name = tinyMat.name.size() == 0 ? "mat" + std::to_string(i) : tinyMat.name;
				emmisiveTex->name = name + "_EMISSIVE";
				mat->texEmisiveMap = name + "_EMISSIVE";
				emmisiveTex->uri = tinyEmmisiveTexImg.uri;
				emmisiveTex->width = tinyEmmisiveTexImg.width;
				emmisiveTex->height = tinyEmmisiveTexImg.height;
				emmisiveTex->component = tinyEmmisiveTexImg.component;
				emmisiveTex->bits = tinyEmmisiveTexImg.bits;
				filepath = Config::gltfFileDirectory + L"\\" + Utils::to_wide_str(emmisiveTex->uri);
				std::unique_ptr<uint8_t[]> wicEMData;
				BREAKIFFAILED(
					DirectX::LoadWICTextureFromFile(
						Graphics::gDevice.Get(),
						filepath.c_str(),
						emmisiveTex->textureResource.ReleaseAndGetAddressOf(),
						wicEMData,
						subresource
					));

				UploadToDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), emmisiveTex->textureResource, emmisiveTex->textureUploader, subresource);
				auto emmisiveTexName = std::wstring(emmisiveTex->name.begin(), emmisiveTex->name.end());
				emmisiveTex->textureUploader->SetName((emmisiveTexName + L"Uploader").c_str());
				emmisiveTex->textureResource->SetName((emmisiveTexName + L"Default").c_str());
				emmisiveTex->textureUploader->SetName(L"emmisiveUploader");
				emmisiveTex->textureResource->SetName(L"emmisiveDefault");
				model.textures[emmisiveTex->name] = std::move(emmisiveTex);
				mat->hasEmissiveTexture = 1;
			}
			else {
				mat->hasEmissiveTexture = 0;
				mat->texEmisiveMap = "";
			}

			model.materials[i+1] = std::move(mat);
		}

		
	}

	void SolveLights(tinygltf::Model& tinyModel, Scene::Model& model, ComPtr<ID3D12GraphicsCommandList> commandList){
		auto& tinylights = tinyModel.lights;
		if (tinylights.size() == 0) {
			//use default directional light 
			
			model.lights.numDirectionalLights = 1;
			model.lights.numPointLights = 0;
			model.lights.numSpotLights = 0;

			auto& directionalLight0 = model.lights.directionalLights[0];
			directionalLight0.color = { 1.0f, 1.0f, 1.0f };
			directionalLight0.lightDirection = { 0.57735f, -0.57735f, 0.57735f };
			directionalLight0.strength = { 0.8f, 0.8f, 0.8f };
			directionalLight0.enabled = 1;
			return;
		}

		
	}

	//TODO: apply animations to children nodes
	void SolveAnimations(tinygltf::Model& tinyModel, Scene::Model& model) {
		auto& tinyAnimations = tinyModel.animations;
		for (auto& tinyAnimation : tinyAnimations) {
			for (auto& channel : tinyAnimation.channels) {
				auto& sampler = tinyAnimation.samplers[channel.sampler];
				auto& node = model.nodes[channel.target_node];
				
				
				
				//path
				if (channel.target_path == "translation") {
					node.animation.hasTranslationAnimation = 1;
					auto& target = node.animation.translationTime;
					{
						tinygltf::Accessor timeAccessor = tinyModel.accessors[sampler.input];
						tinygltf::BufferView& timeBufferview = tinyModel.bufferViews[timeAccessor.bufferView];
						auto& timeBuffer = tinyModel.buffers[timeBufferview.buffer];
						auto numElements = 0;
						if (timeAccessor.type != TINYGLTF_TYPE_SCALAR) {
							__debugbreak();
						}
						numElements = 1;
						auto stride = 0;
						if (timeAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
							stride = 4;
						}
						else {
							__debugbreak();
						}
						auto start = timeBufferview.byteOffset + timeAccessor.byteOffset;
						auto end = start + timeAccessor.count * stride * numElements;

						auto itBegin = std::next(timeBuffer.data.begin(), start);
						auto itEnd = std::next(timeBuffer.data.begin(), end);

						std::vector<byte> timeTemp{ itBegin , itEnd };
						target.resize(timeAccessor.count);
						std::memcpy(target.data(), timeTemp.data(), timeTemp.size());
					}

					auto& target2 = node.animation.translationAnimation;
					{
						tinygltf::Accessor actionAccessor = tinyModel.accessors[sampler.output];
						tinygltf::BufferView& actionBufferview = tinyModel.bufferViews[actionAccessor.bufferView];
						auto& actionBuffer = tinyModel.buffers[actionBufferview.buffer];
						auto numElements = 0;
						if (actionAccessor.type != TINYGLTF_TYPE_VEC3) {
							__debugbreak();
						}
						numElements = 3;
						auto stride = 0;
						if (actionAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
							stride = 4;
						}
						else {
							__debugbreak();
						}
						auto start = actionBufferview.byteOffset + actionAccessor.byteOffset;
						auto end = start + actionAccessor.count * stride * numElements;

						auto itBegin = std::next(actionBuffer.data.begin(), start);
						auto itEnd = std::next(actionBuffer.data.begin(), end);

						std::vector<byte> temp{ itBegin , itEnd };
						target2.resize(actionAccessor.count);
						std::memcpy(target2.data(), temp.data(), temp.size());

					}

				}
				else if (channel.target_path == "rotation") {
					node.animation.hasRotationAnimation = 1;
					auto& target  = node.animation.rotationTime;
					{
						tinygltf::Accessor timeAccessor = tinyModel.accessors[sampler.input];
						tinygltf::BufferView& timeBufferview = tinyModel.bufferViews[timeAccessor.bufferView];
						auto& timeBuffer = tinyModel.buffers[timeBufferview.buffer];
						auto numElements = 0;
						if (timeAccessor.type != TINYGLTF_TYPE_SCALAR) {
							__debugbreak();
						}
						numElements = 1;
						auto stride = 0;
						if (timeAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
							stride = 4;
						}
						else {
							__debugbreak();
						}
						auto start = timeBufferview.byteOffset + timeAccessor.byteOffset;
						auto end = start + timeAccessor.count * stride * numElements;

						auto itBegin = std::next(timeBuffer.data.begin(), start);
						auto itEnd = std::next(timeBuffer.data.begin(), end);

						std::vector<byte> timeTemp{ itBegin , itEnd };
						target.resize(timeAccessor.count);
						std::memcpy(target.data(), timeTemp.data(), timeTemp.size());

					}

					auto& target2 = node.animation.rotationAnimation;
					{
						tinygltf::Accessor actionAccessor = tinyModel.accessors[sampler.output];
						tinygltf::BufferView& actionBufferview = tinyModel.bufferViews[actionAccessor.bufferView];
						auto& actionBuffer = tinyModel.buffers[actionBufferview.buffer];
						auto numElements = 0;
						if (actionAccessor.type != TINYGLTF_TYPE_VEC4) {
							__debugbreak();
						}
						numElements = 4;
						auto stride = 0;
						if (actionAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
							stride = 4;
						}
						else {
							__debugbreak();
						}
						auto start = actionBufferview.byteOffset + actionAccessor.byteOffset;
						auto end = start + actionAccessor.count * stride * numElements;

						auto itBegin = std::next(actionBuffer.data.begin(), start);
						auto itEnd = std::next(actionBuffer.data.begin(), end);

						std::vector<byte> temp{ itBegin , itEnd };
						target2.resize(actionAccessor.count);
						std::memcpy(target2.data(), temp.data(), temp.size());
					}

				}
				else if (channel.target_path == "scale") {
					node.animation.hasScalingAnimation = 1;

				}
				else {//weights

				}
				
			}
		}
	}

	int translate(tinygltf::Model& tinyModel, Scene::Model& model, ComPtr<ID3D12GraphicsCommandList> commandList) {
		const tinygltf::Scene& tinyscene = tinyModel.scenes[tinyModel.defaultScene];
		//model.buffers.resize(tinyModel.buffers.size());
		model.meshes.resize(tinyModel.meshes.size());
		model.nodes.resize(tinyModel.nodes.size());
		model.materials.resize(tinyModel.materials.size()+1);

		model.numNodes = (UINT)tinyModel.nodes.size();
		model.numMeshes = (UINT)tinyModel.meshes.size();
		model.numMaterials = (UINT)tinyModel.materials.size()+1;
		model.numTextures = (UINT)tinyModel.textures.size();

		//DirectX::XMLoadFloat4x4(model.);

		//TO EDIT: NO NEED TO MOVE OVER ALL BUFFERS, AS THEY WILL BE CLASSIFIED AND TRANSFERED TO GPU RESOURCE
		//move over all buffers 
		//for (UINT i = 0; i < tinyModel.buffers.size(); i++) {
		//	model.buffers[i] = std::move(tinyModel.buffers[i].data);
		//}

		//iterate over all nodes 
		std::vector<bool> visited;
		visited.resize(model.nodes.size(),false);
		for (UINT i = 0; i < tinyscene.nodes.size(); i++) {
			auto nodeIndex = tinyscene.nodes[i];
			if (!visited[nodeIndex]) {
				visited[nodeIndex] = true;
				SolveNodes(tinyModel, nodeIndex, model, Math::IdentityMatrix(), visited);
			}
			
		}

		SolveMaterials(tinyModel, model, commandList);
		SolveLights(tinyModel, model, commandList);
		SolveAnimations(tinyModel, model);
		return 1;
	}

	
	void CreateCubeMapGeo(ComPtr<ID3D12GraphicsCommandList> commandList) {


		std::vector<float> vertices =
		{

				1.0,  - 1.0,    1.0 ,
			  - 1.0,    1.0,  - 1.0 ,
				1.0,    1.0,  - 1.0 ,
				1.0,  - 1.0,  - 1.0 ,
			  - 1.0,  - 1.0,    1.0 ,
			  - 1.0,    1.0,    1.0 ,
				1.0,    1.0,    1.0 ,
			  - 1.0,  - 1.0,  - 1.0
		};

		std::array<std::uint16_t, 36> indices =
		{

			7, 1, 2,
			7, 2, 3,
			4, 6, 5,
			4, 0, 6,
			4, 5, 1,
			4, 1, 7,
			3, 2, 6,
			3, 6, 0,
			1, 5, 6,
			1, 6, 2,
			4, 7, 3,
			4, 3, 0

		};
		Cubemap::indexCount = indices.size();
		Cubemap::indexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t) ;
		Cubemap::vertexPosBufferByteSize = (UINT)vertices.size() * sizeof(float);

		Cubemap::indexBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), indices.data(), Cubemap::indexBufferByteSize, Cubemap::indexUploader);
		Cubemap::vertexPosBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), vertices.data(), Cubemap::vertexPosBufferByteSize, Cubemap::vertexPosUploader);

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

		if (model.indexBufferByteSize > 0) {
			model.indexBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.indexBufferCPU.data(), model.indexBufferByteSize, indexUploader);
		}
		model.vertexPosBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.vertexPosBufferCPU.data(), model.vertexPosBufferByteSize, vertexPosUploader);
		
		if (model.vertexNormalBufferByteSize) {
			model.vertexNormalBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.vertexNormalBufferCPU.data(), model.vertexNormalBufferByteSize, vertexNormalUploader);
		}
		if (model.vertexTexCordBufferByteSize > 0) {
			model.vertexTexCordBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.vertexTexCordBufferCPU.data(), model.vertexTexCordBufferByteSize, vertexTexCordUploader);
		}

		if (model.vertexTangentBufferByteSize > 0) {
			model.vertexTangentBufferGPU = CreateDefaultBuffer(Graphics::gDevice.Get(), commandList.Get(), model.vertexTangentBufferCPU.data(), model.vertexTangentBufferByteSize, vertexTangentUploader);
		}


		CreateCubeMapGeo(commandList);

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


		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));



		return defaultBuffer;
	}
	

}
