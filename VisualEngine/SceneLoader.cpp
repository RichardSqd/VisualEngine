#include "pch.h"
#include "SceneLoader.h"
#include "tiny_gltf.h"

namespace Scene {
	void solveMeshs(tinygltf::Model& tinyModel, tinygltf::Mesh& tinyMesh, Scene::Model& model) {
		for (UINT i = 0; i < tinyModel.bufferViews.size(); i++) {
			const tinygltf::BufferView& tinyBufferView = tinyModel.bufferViews[i];
			if (tinyBufferView.target == 0) {
				continue;
			}

			const tinygltf::Buffer& tinyBuffer = tinyModel.buffers[tinyBufferView.buffer];
			size_t size = tinyBuffer.data.size();
			size_t offset = tinyBufferView.byteOffset;
		}

		for (UINT i = 0; i < tinyMesh.primitives.size(); i++) {
			tinygltf::Primitive primitive = tinyMesh.primitives[i];
			tinygltf::Accessor indexAccessor = tinyModel.accessors[primitive.indices];

			for (auto& attribute : primitive.attributes) {
				tinygltf::Accessor accessor = tinyModel.accessors[attribute.second];
				int stride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attribute.first.compare("POSITION") == 0) vaa = 0;
				if (attribute.first.compare("NORMAL") == 0) vaa = 1;
				if (attribute.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (vaa < 0)  ASSERT(0);
					
				
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
	void solveNodes(tinygltf::Model& tinyModel, tinygltf::Node& tinyNode, Scene::Model& model) {
		//validate current mesh
		if ((tinyNode.mesh >= 0) && (tinyNode.mesh < tinyModel.meshes.size())) {
			solveMeshs(tinyModel, tinyModel.meshes[tinyNode.mesh], model);
		}

		for (UINT i = 0; i < tinyNode.children.size(); i++) {
			ASSERT((tinyNode.children[i] >= 0) && (tinyNode.children[i] < tinyModel.nodes.size()));
			solveNodes(tinyModel, tinyModel.nodes[tinyNode.children[i]], model);
		}

		

	}

	int translate(tinygltf::Model& tinyModel, Scene::Model& model) {
		const tinygltf::Scene& scene = tinyModel.scenes[tinyModel.defaultScene];
		//iterate over all nodes 
		for (UINT i = 0; i < scene.nodes.size(); i++) {
			solveNodes(tinyModel, tinyModel.nodes[scene.nodes[i]], model);
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