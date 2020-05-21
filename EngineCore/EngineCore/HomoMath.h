#pragma once
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

//Math的几大部分:
//1.三维和四位向量
//2.3x3和4x4矩阵
//3.四元数
//4.解方程辅助

namespace HomoMath {
	//通过一些tag来获得特殊的数学对象

	struct ZERO {};
	struct IDENTITY {};
	struct XUINT {};
	struct YUINT {};
	struct ZUINT {};
	struct WUINT {};

	template<typename MathItem, typename Tag>
	MathItem GetMathItemByTag() {
		return MathItem(Tag());
	}
}

namespace HomoMath {
	//1.Math的Vector向量部分

	using namespace DirectX;

	struct Vector4;

	struct Scalar {
		XMVECTOR v;
		Scalar(float n) { v = XMVectorReplicate(n); }
		Scalar(const XMVECTOR& vec) :v(vec) {}
		Scalar(const Scalar& scl) :v(scl.v) {}
		Scalar(ZERO) { v = XMVectorReplicate(0); }
		Scalar(IDENTITY) { v = XMVectorReplicate(1); }

		Scalar operator*(Scalar rhs) {
			return Scalar(XMVectorMultiply(v, rhs.v));
		}
		Scalar operator+(Scalar rhs) {
			return Scalar(XMVectorAdd(v, rhs.v));
		}
		Scalar operator-(Scalar rhs) {
			return Scalar(XMVectorSubtract(v, rhs.v));
		}
		Scalar operator/(Scalar rhs) {
			return Scalar(XMVectorDivide(v, rhs.v));
		}

		float getNum() { float temp; XMStoreFloat(&temp, v); return temp; }
	};

	inline Scalar operator*(Scalar lhs, float rhs) {
		return lhs * Scalar(rhs);
	}

	inline Scalar operator*(float lhs, Scalar rhs) {
		return Scalar(lhs) * rhs;
	}

#define ComputeWithScalar(Vector)\
	inline Vector operator*(Scalar lhs,Vector rhs) {\
		return Vector(XMVectorMultiply(lhs.v, rhs.v));\
	}\
	inline Vector operator*(Vector lhs, Scalar rhs) {\
		return Vector(XMVectorMultiply(lhs.v, rhs.v));\
	}\
	inline Vector operator*(Vector lhs, float rhs) {\
		return lhs * Scalar(rhs);\
	}\
	inline Vector operator*(float lhs, Vector rhs) {\
		return Scalar(lhs) * rhs;\
	}\
	inline Vector operator+(Vector lhs, Scalar rhs) {\
		return XMVectorAdd(lhs.v, rhs.v);\
	}\
	inline Vector operator+(Scalar lhs, Vector rhs) {\
		return XMVectorAdd(rhs.v, lhs.v);\
	}\
	inline Vector operator+(float lhs, Vector rhs) {\
		return Scalar(lhs) + rhs;\
	}\
	inline Vector operator+(Vector lhs, float rhs) {\
		return Scalar(rhs) + lhs;\
	}\
	inline Vector operator-(Vector lhs,Scalar rhs){\
		return Vector(XMVectorSubtract(lhs.v,rhs.v));\
	}\
	inline Vector operator-(Scalar lhs,Vector rhs){\
		return Vector(XMVectorSubtract(lhs.v,rhs.v));\
	}\
	inline Vector operator-(Vector lhs,float rhs){\
		return lhs - Scalar(rhs);\
	}\
	inline Vector operator-(float lhs,Vector rhs){\
		return Scalar(lhs) - rhs;\
	}\
	inline Vector operator/(Vector lhs,Scalar rhs){\
		return XMVectorDivide(lhs.v,rhs.v);\
	}\
	inline Vector operator/(Vector lhs,float rhs){\
		return lhs / Scalar(rhs);\
	}

	//Vector3的最后一位一定是0,Vector3是16位对齐的
	struct Vector3 {
		XMVECTOR v;
		Vector3(float x, float y, float z) :v(XMVectorSet(x, y, z, 0)) { }
		Vector3(const Vector3& vec) :v(vec.v) {}
		//Vector3(const Vector4& vec) { XMFLOAT3 temp; XMStoreFloat3(&temp, vec.v); v = XMLoadFloat3(&temp); }
		Vector3(const XMVECTOR& vec) { v = XMVectorPermute<0, 1, 2, 7>(vec, XMVectorReplicate(0)); }
		Vector3(ZERO) { v = XMVectorReplicate(0); }
		Vector3(XUINT) { v = XMVectorSet(1.f, 0, 0, 0); }
		Vector3(YUINT) { v = XMVectorSet(0, 1.f, 0, 0); }
		Vector3(ZUINT) { v = XMVectorSet(0, 0, 1.f, 0); }

		Scalar getX() { return XMVectorSplatX(v); }
		Scalar getY() { return XMVectorSplatY(v); }
		Scalar getZ() { return XMVectorSplatZ(v); }
		void setX(float x) { v = XMVectorPermute<4, 1, 2, 3>(v, XMVectorReplicate(x)); }
		void setY(float y) { v = XMVectorPermute<0, 5, 2, 3>(v, XMVectorReplicate(y)); }
		void setZ(float z) { v = XMVectorPermute<0, 1, 6, 3>(v, XMVectorReplicate(z)); }

		Vector3 operator+(Vector3 rhs) {
			return Vector3(XMVectorAdd(v, rhs.v));
		}
		Vector3 operator-(Vector3 rhs) {
			return Vector3(XMVectorSubtract(v, rhs.v));
		}

		Vector3 normalize() {
			return XMVector3Normalize(v);
		}
		Scalar length() {
			return XMVector3Length(v);
		}
	};

	ComputeWithScalar(Vector3)

		inline Scalar dot(Vector3 lhs, Vector3 rhs) {
		return XMVectorSum(XMVectorMultiply(lhs.v, rhs.v));
	}

	inline Vector3 cross(Vector3 lhs, Vector3 rhs) {
		return XMVector3Cross(lhs.v, rhs.v);
	}

	struct Vector4 {
		XMVECTOR v;

		Vector4(float x, float y, float z, float w) : v(XMVectorSet(x, y, z, w)) {}
		Vector4(const XMVECTOR& vec) :v(vec) {}
		Vector4(const Vector3& vec) :v(vec.v) {}
		Vector4(ZERO) { v = XMVectorReplicate(0); }
		Vector4(XUINT) { v = XMVectorSet(1.f, 0, 0, 0); }
		Vector4(YUINT) { v = XMVectorSet(0, 1.f, 0, 0); }
		Vector4(ZUINT) { v = XMVectorSet(0, 0, 1.f, 0); }
		Vector4(WUINT) { v = XMVectorSet(0, 0, 0, 1.f); }

		Scalar getX() { return XMVectorSplatX(v); }
		Scalar getY() { return XMVectorSplatY(v); }
		Scalar getZ() { return XMVectorSplatZ(v); }
		Scalar getW() { return XMVectorSplatW(v); }
		void setX(float x) { v = XMVectorPermute<4, 1, 2, 3>(v, XMVectorReplicate(x)); }
		void setY(float y) { v = XMVectorPermute<0, 5, 2, 3>(v, XMVectorReplicate(y)); }
		void setZ(float z) { v = XMVectorPermute<0, 1, 6, 3>(v, XMVectorReplicate(z)); }
		void setW(float w) { v = XMVectorPermute<0, 1, 2, 7>(v, XMVectorReplicate(w)); }

		Vector4 operator+(Vector4 rhs) {
			return Vector3(XMVectorAdd(v, rhs.v));
		}
		Vector4 operator-(Vector4 rhs) {
			return Vector4(XMVectorSubtract(v, rhs.v));
		}

		//将vector4的最后一位变为1
		Vector4 normByW() {
			return XMVectorDivide(v, XMVectorSplatW(v));
		}
	};

	ComputeWithScalar(Vector4);
}
