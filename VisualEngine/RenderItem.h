#pragma once

class RenderItem {

public:
	RenderItem();
	RenderItem(const RenderItem& rhs);


private:

	DirectX::XMFLOAT4X4 mWorld;
	DirectX::XMFLOAT4X4 mTextureTransform;

	int mNumFrameDirty;
	UINT mObjIndex; 
	
	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


};