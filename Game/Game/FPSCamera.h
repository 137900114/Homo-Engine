#pragma once
#include "SceneCamera.h"

namespace Game {
	class FPSCamera
	{
	public:

		void attach(SceneCamera* camera);

		FPSCamera(SceneCamera* camera = nullptr) {
			attach(camera);
		}

		void tick();

		inline SceneCamera* getSceneCamera() {
			return camera;
		}
		void walk(float distance);
		void strafe(float distance);

		void rotateY(float angle);
		void rotateX(float angle);

		void look(Vector3 Position);
	private:
		void updateAxis();
		void updateAngle();

		SceneCamera* camera;
		
		Vector3 Position;

		Vector3 lookAt;
		Vector3 up;
		Vector3 bioAxis;

		float theta = 0.;
		float phi = 0.;
	};
}

