#pragma once
#include "Math.h"
#include "uuid.h"

namespace Game {


	class Camera {
	public:

		//CameraBuffer GetCameraBuffer();
		
		void SetAspect(float aspectRatio) {
			aspect = aspectRatio;
			UpdateProjection();
		}
		float GetAspect() { return aspect; }

		void SetFovY(float fovY) {
			this->fovY = fovY;
			UpdateProjection();
		}
		float GetFovY() { return fovY; }

		void SetNearPlane(float near) {
			this->near = near;
			UpdateProjection();
		}
		float GetNearPlane() { return near; }

		void SetFarPlane(float far) {
			this->far = far;
			UpdateProjection();
		}
		float GetFarPlane() { return far; }

		void SetRenderTarget(UUID rtv) {
			renderTarget = rtv;
		}

		void SetDepthStencil(UUID dsv) {
			depthStencil = dsv;
		}

		void SetProjection(float near,float far,float aspect,float fovY) {
			this->near = near;
			this->far = far;
			this->aspect = aspect;
			this->fovY = fovY;
			UpdateProjection();
		}

		UUID GetRenderTarget() { return renderTarget; }
		UUID GetDepthStencil() { return depthStencil; }

		Mat4x4 GetProj() { return proj; }
		Mat4x4 GetInvProj() { return invProj; }

		/*void LookAt(Vector3 Position,Vector3 target,Vector3 up);
		void Forward(float dis) {
			Translate(Vector2(dis, 0.));
		}
		void Strafe(float dis) {
			Translate(Vector2(0., dis));
		}

		void Translate(Vector2 pos);
		void Transform(Mat4x4 mat);

		void Pitch(float angle);
		void RotateY(float angle);*/
		
	private:

		//void UpdateViewMatrix();

		void UpdateProjection();

		/*Vector3 Position;
		Vector3 Right;
		Vector3 Up;
		Vector3 Look;*/

		float aspect;
		float fovY;
		float near;
		float far;


		//CameraBuffer buffer;
		UUID   renderTarget;
		UUID   depthStencil;
		//UUID   cameraViewBuffer;

		Mat4x4 proj;
		Mat4x4 invProj;
	};
}