#pragma once

namespace Math {
	extern inline UINT Alignment256(UINT64 byteSize);
	extern inline DirectX::XMMATRIX IdentityMatrix();
	extern inline DirectX::XMMATRIX VectorToMatrix(const std::vector<double> v);
	extern inline DirectX::XMMATRIX QuaternionToMatrix(const std::vector<double> v);
	extern inline DirectX::XMMATRIX Scale(const std::vector<double> v);
	extern inline DirectX::XMMATRIX Trans(const std::vector<double> v);
	extern inline float Clamp(const float& x, const float& low, const float& high);
	extern const float PI;
	extern DirectX::XMMATRIX InverseTranspose(const DirectX::CXMMATRIX matrix);
}