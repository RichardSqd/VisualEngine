#include "pch.h"
#include "Graphics.h"
#include "Math.h"
const float MathHelper::PI = 3.1415926535f;

inline DirectX::XMMATRIX MathHelper::IdentityMatrix() {
	return DirectX::XMMATRIX( 1,0,0,0,
								0,1,0,0,
								0,0,1,0,
								0,0,0,1);

}

inline DirectX::XMMATRIX MathHelper::VectorToMatrix(const std::vector<double> v) {
	ASSERT(v.size() == 16);

	return DirectX::XMMATRIX( v[0], v[1], v[2], v[3],
								v[4], v[5], v[6], v[7],
								v[8], v[9], v[10], v[11],
								v[12], v[13], v[14], v[15]);
}

inline DirectX::XMMATRIX MathHelper::QuaternionToMatrix(const std::vector<double> v) {
	if (v.size() != 4)  return MathHelper::IdentityMatrix();
	float q0 = v[0];
	float q1 = v[0];
	float q2 = v[0];
	float q3 = v[0];

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




inline DirectX::XMMATRIX MathHelper::Scale(const std::vector<double> v) {
	if (v.size() != 3)  return MathHelper::IdentityMatrix();

	return DirectX::XMMATRIX(   v[0],       0,     0,    0,
								   0,    v[1],     0,    0,
								   0,       0,  v[2],    0,
								   0,       0,     0,  v[3]);
}
inline DirectX::XMMATRIX MathHelper::Trans(const std::vector<double> v) {
	if (v.size() != 3)  return MathHelper::IdentityMatrix();
	
	return DirectX::XMMATRIX(	   1,       0,     0,    v[0],
								   0,       1,     0,    v[1],
								   0,       0,     1,    v[2],
								   0,       0,     0,    1);

}