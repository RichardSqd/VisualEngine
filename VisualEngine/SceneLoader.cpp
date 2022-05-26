#include "pch.h"
#include "SceneLoader.h"
#include "tiny_gltf.h"


namespace Scene {
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
			Primitives& primitive = mesh.primitives[i];
			{ //Index buffer
				tinygltf::Accessor indexAccessor = tinyModel.accessors[tinyPrimitive.indices];
				tinygltf::BufferView& indexBufferview =  tinyModel.bufferViews[indexAccessor.bufferView];

				ASSERT(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
				primitive.indexBuffer = indexBufferview.buffer;
				primitive.format = DXGI_FORMAT_R16_UINT;
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
				localToObject = MathHelper::VectorToMatrix(tinyNode.matrix) * localToObject;
			}
			else {
				DirectX::XMMATRIX scale = MathHelper::Scale(tinyNode.scale);
				DirectX::XMMATRIX translate = MathHelper::Trans(tinyNode.translation);
				DirectX::XMMATRIX rotate = MathHelper::QuaternionToMatrix(tinyNode.rotation);
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
			SolveNodes(tinyModel, scene.nodes[i], model, MathHelper::IdentityMatrix());
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
}