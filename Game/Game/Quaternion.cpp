#include "Quaternion.h"
//#include <xmmintrin.h>

using namespace Game;
/*
__m128 _quaternion_mul(__m128 lhs,__m128 rhs) {

}

struct _PackedMat4x4 {
	__m128 m[4];
};

inline _PackedMat4x4 _load_mat(const Game::Mat4x4& mat) {
	_PackedMat4x4 target;
	target.m[0] = _mm_load_ps(mat.a[0]);
	target.m[1] = _mm_load_ps(mat.a[1]);
	target.m[2] = _mm_load_ps(mat.a[2]);
	target.m[3] = _mm_load_ps(mat.a[3]);

	return target;
}

inline Game::Mat4x4 _store_mat4(_PackedMat4x4 mat) {
	Game::Mat4x4 target;
	_mm_storeu_ps(target.a[0], mat.m[0]);
	_mm_storeu_ps(target.a[1], mat.m[1]);
	_mm_storeu_ps(target.a[2], mat.m[2]);
	_mm_storeu_ps(target.a[3], mat.m[3]);

	return target;
}


_PackedMat4x4 _cast_quaternion_to_mat(__m128 vec) {
	__m128 sqr = _mm_mul_ps(vec, vec);
	float constOne[] = { 0.,1.,1.,1. };
	float constTwo[] = { 0.,2.,2.,2. };
	float constInv[] = { 0.,1.,0.,-1.};
	__m128 one = _mm_load_ps(constOne);
	__m128 two = _mm_load_ps(constTwo);
	__m128 inv = _mm_load_ps(constInv);


	__m128 temp1 = _mm_shuffle_ps(sqr, sqr, _MM_SHUFFLE(1, 3, 2, 0));
	temp1 = _mm_add_ps(sqr, temp1);
	temp1 = _mm_mul_ps(temp1, two);

	//sqr = (0.,1 - y*y*2 - x*x*2,1 - y*y*2 - z*z*2,1 - z*z*2 - x*x*2);
	sqr = _mm_sub_ps(one, temp1);


	__m128 nmn = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 3, 2, 0));
	__m128 wmn = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));

	//nmn = (0.,2*x*y,2*y*x,2*z*x);
	nmn = _mm_mul_ps(_mm_mul_ps(vec, nmn), two);
	//wmn = (0.,2*x*w,2*y*w,2*z*w);
	wmn = _mm_mul_ps(_mm_mul_ps(vec, wmn), two);

	_PackedMat4x4 mat;

	mat.m[0] = _mm_shuffle_ps(wmn, wmn, _MM_SHUFFLE(0, 3, 2, 0));
	mat.m[0] = _mm_mul_ps(mat.m[0], inv);
	temp1 = _mm_shuffls_ps(nmn,nmn,);
}
*/

Mat4x4 Quaternion::Mat(Vector3 Pos) {
	float xsqr = x * x, ysqr = y * y, zsqr = z * z;
	float xy = x * y, yz = y * z, xz = x * z;
	float xw = x * w, yw = y * w, zw = z * w;

	Mat4x4 result(
		1.f - (ysqr + zsqr) * 2.f, 2.f * (xy + zw), 2.f * (xz - yw), Pos[0],
		2.f * (xy - zw), 1.f - (xsqr + zsqr) * 2.f, 2.f * (yz + xw), Pos[1],
		2.f * (xz + yw), 2.f * (yz - xw), 1.f - (xsqr + ysqr) * 2.f, Pos[2],
		0.f, 0.f, 0., 1.

	);
	return result;
}


void Quaternion::CastByMatrix(const Mat4x4& mat) {
	w = sqrt((mat.a[0][0] + mat.a[1][1] + mat.a[2][2] + 1.f) * .25f);

	x = (mat.a[1][2] - mat.a[2][1]) / (4.f * w);
	y = (mat.a[0][2] - mat.a[2][0]) / (4.f * w);
	z = (mat.a[0][1] - mat.a[1][0]) / (4.f * w);
}

/*
void Quaternion::CastByMatrix(const Mat4x4& mat) {
	Mat4x4 matCopy(mat);
	matCopy.a[0][0] -= 1.f, matCopy.a[1][1] -= 1.f, matCopy.a[2][2] -= 1.f;


}

*/


Quaternion  Game::mul(const Quaternion& lhs,const Quaternion& rhs) {
	return Quaternion(
		lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
		lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.w,
		lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w
	);
}

Vector4 Game::mul(const Quaternion& lhs,const Vector4& rhs) {
	return Vector4(
		lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
		lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.w,
		lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w
	);
}


Quaternion Game::transform(const Vector3& lhs,const Quaternion& rhs) {
	return mul(lhs, Vector4(0., lhs.x, lhs.y, lhs.z));
}

