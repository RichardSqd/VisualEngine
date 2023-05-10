#include "pch.h"
#include "Model.h"

namespace Scene {
	DirectX::XMFLOAT4 axis = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	float angle = 0.0;
	DirectX::XMFLOAT4 sceneScaling = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	DirectX::XMFLOAT4 sceneTranslation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	Mesh::Mesh() {

	}

	Primitive::Primitive()
		:	iformat(DXGI_FORMAT_R32_UINT),
			ibOffset(0),
			indexBufferByteSize(0),
			indexCount(0),

			vbPosOffset(0),
			vertexBufferPosByteSize(0),
			vertexCount(0),

			vbTexOffset(0),
			vertexBufferTexCordByteSize(0),

			vbNormalOffset(0),
			vbNormalBufferByteSize(0),

			vbTangentOffset(0),
			vbTangentBufferByteSize(0),
			matIndex(-1){}

	Node::Node()
		:	toWorldmatrix(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0),
			rotation(0.0, 0.0, 0.0, 0.0),
			scale(0.0,0.0,0.0),
			translation(0.0,0.0,0.0),
			mesh(-1),
			indexCount(0),
			ibOffset(0),
			vbOffset(0),
			numFrameDirty(0){
	}


}