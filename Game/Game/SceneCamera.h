#pragma once
#include "SceneRootNode.h"
#include "uuid.h"
#include "Camera.h"
#include "ShaderDataStructure.h"
#include "Material.h"

namespace Game {

	struct SceneCamera : public SceneRootNode {
		SceneCamera(const char* name, Scene* scene, Camera& camera) :
			SceneRootNode(name, scene, CAMERA), camera(camera) {

			cameraBuffer.data.resize(sizeof(ShaderStanderdCamera));
			cameraBuffer.regID = 1;
			cameraBuffer.updated = true;
			cameraBufferPtr = reinterpret_cast<ShaderStanderdCamera*>(cameraBuffer.data.data);
		}

		void SetAspect(float aspect) { camera.SetAspect(aspect); UpdateCameraBufferProjection(); }
		float GetAspect() { return camera.GetAspect(); }

		void SetFovY(float fovY) { camera.SetFovY(fovY);  UpdateCameraBufferProjection(); }
		float GetFovY() { return camera.GetFovY(); }

		Game::UUID GetRenderTarget() { return camera.GetRenderTarget(); }
		Game::UUID GetDepthStencil() { return camera.GetDepthStencil(); }

		void SetRenderTarget(Game::UUID renderTarget) { camera.SetRenderTarget(renderTarget); }
		void SetDepthStencil(Game::UUID depthStencil) { camera.SetDepthStencil(depthStencil); }

		float GetFarPlane() { return camera.GetFarPlane(); }
		void SetFarPlane(float far) { camera.SetFarPlane(far);  UpdateCameraBufferProjection(); }

		float GetNearPlane() { return camera.GetNearPlane(); }
		void SetNearPlane(float near) { camera.SetNearPlane(near);  UpdateCameraBufferProjection(); }

		void SetProjection(float near, float far, float aspect, float fovY) {
			camera.SetProjection(near, far, aspect, fovY);
			UpdateCameraBufferProjection();
		}

		void UpdateCameraBufferView();

		CBuffer* GetCameraCBuffer();

		virtual void initialize() override;

	private:
		void UpdateCameraBufferProjection();

		Camera camera;
		ShaderStanderdCamera* cameraBufferPtr;
		CBuffer cameraBuffer;
	};
}