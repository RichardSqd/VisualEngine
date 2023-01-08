#pragma once

namespace Math {
	extern inline DirectX::XMMATRIX IdentityMatrix();
	extern inline DirectX::XMMATRIX VectorToMatrix(const std::vector<double> v);
	extern inline DirectX::XMMATRIX QuaternionToMatrix(const std::vector<double> v);
	extern inline DirectX::XMMATRIX Scale(const std::vector<double> v);
	extern inline DirectX::XMMATRIX Trans(const std::vector<double> v);
	extern inline float AspectRatio();
	extern const float PI;
	
}