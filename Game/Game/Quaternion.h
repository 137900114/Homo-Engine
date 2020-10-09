#pragma once
#include "Vector.h"
#include "Math.h"

namespace Game {
	struct Quaternion {
		union {
			struct {
				float w, x, y, z;
			};
			float raw[4];
			Vector4 vec;
		};

		Quaternion(float w,float x,float y,float z):
		 x(x),y(y),z(z),w(w){}

		Quaternion(Vector4 vec):vec(vec) {

		}

		Quaternion(float* num) {
			w = num[0], x = num[1], y = num[2], z = num[3];
		}
		
		Quaternion(const Mat4x4& Rotation) {
			CastByMatrix(Rotation);
		}
		Quaternion(Vector3 EulerAngle) {
			CastByMatrix(MatrixRotation(EulerAngle));
		}
		

		inline Mat4x4 Mat() {
			return Mat(Vector3(0.,0.,0.));
		}
		Mat4x4 Mat(Vector3 Pos);


		Vector3 EulerAngle() {
			Vector3 angle,p,s;
			UnpackTransfrom(Mat(), p, angle, s);
			return angle;
		}


		float length() {
			return Game::length(vec);
		}

		Quaternion operator+(const Quaternion& other) {
			return Quaternion(vec + other.vec);
		}

		Quaternion operator*(float scale) {
			return Quaternion(vec * scale);
		}



	private:
		//
		void CastByMatrix(const Mat4x4& mat);
	};

	Quaternion    mul(const Quaternion& lhs,const Quaternion& rhs);
	Vector4       mul(const Quaternion& lhs,const Vector4&    rhs);
	Quaternion    transform(const Vector3& lhs, const Quaternion& rhs);
}