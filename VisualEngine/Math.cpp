#include "pch.h"
#include "Graphics.h"
#include "Math.h"
const float Math::PI = 3.1415926535f;

inline UINT Math::Alignment256(UINT64 byteSize) {
	return (byteSize + 255) & ~255;
}

inline DirectX::XMMATRIX Math::IdentityMatrix() {
	return DirectX::XMMatrixIdentity();

}

inline DirectX::XMMATRIX Math::VectorToMatrix(const std::vector<double> v) {
	ASSERT( ((int)v.size() == 16) );

	return DirectX::XMMATRIX((float)v[0], (float)v[1], (float)v[2], (float)v[3],
		(float)v[4], (float)v[5], (float)v[6], (float)v[7],
		(float)v[8], (float)v[9], (float)v[10], (float)v[11],
		(float)v[12], (float)v[13], (float)v[14], (float)v[15]);
}

inline DirectX::XMMATRIX Math::QuaternionToMatrix(const std::vector<double> v) {
	if (v.size() != 4)  return Math::IdentityMatrix();
	float q0 = (float)v[0];
	float q1 = (float)v[0];
	float q2 = (float)v[0];
	float q3 = (float)v[0];

	float q00 = 2 * (q0 * q0 + q1 * q1) - 1;
	float q01 = 2 * (q1 * q2 - q0 * q3);
	float q02 = 2 * (q1 * q3 + q0 * q2);
	float q03 = 0;

	float q10 = 2 * (q1 * q2 + q0 * q3);
	float q11 = 2 * (q0 * q0 + q2 * q2) - 1;
	float q12 = 2 * (q2 * q3 - q0 * q1);
	float q13 = 0;

	float q20 = 2 * (q1 * q3 - q0 * q2);
	float q21 = 2 * (q2 * q3 + q0 * q1);
	float q22 = 2 * (q0 * q0 + q3 * q3) - 1;
	float q23 = 0;

	float q30 = 0;
	float q31 = 0;
	float q32 = 0;
	float q33 = 1;

	return DirectX::XMMATRIX(   q00, q01, q02, q03,
								q10, q11, q12, q13,
								q20, q21, q22, q23,
								q30, q31, q32, q33);
}




inline DirectX::XMMATRIX Math::Scale(const std::vector<double> v) {
	if (v.size() != 3)  return Math::IdentityMatrix();

	return DirectX::XMMATRIX((float)v[0],       0,     0,    0,
								   0, (float)v[1],     0,    0,
								   0,       0, (float)v[2],    0,
								   0,       0,     0, (float)1);
}
inline DirectX::XMMATRIX Math::Trans(const std::vector<double> v) {
	if (v.size() != 3)  return Math::IdentityMatrix();
	
	return DirectX::XMMATRIX(	   1,       0,     0,    0,
								   0,       1,     0,    0,
								   0,       0,     1,    0,
			(float)v[0], (float)v[1], (float)v[2], (float)1);

}

float Math::Clamp(const float& x, const float& low, const float& high)
{
	return x < low ? low : (x > high ? high : x);
}

extern DirectX::XMMATRIX Math::InverseTranspose(const DirectX::CXMMATRIX matrix) {
	DirectX::XMMATRIX A = matrix;
	A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);;
	DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(&det, A));
}

