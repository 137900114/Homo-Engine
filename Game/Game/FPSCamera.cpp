#include "FPSCamera.h"


namespace Game {


	void FPSCamera::tick() {
		if (camera == nullptr) return;
		Mat4x4 lookAtMat = Mat4x4(
			bioAxis[0],up[0] , lookAt[0] ,Position[0],
			bioAxis[1] ,up[1] ,lookAt[1] ,Position[1],
			bioAxis[2] ,up[2] ,lookAt[2] ,Position[2],
			0         ,0     ,0          ,1
		);
		camera->transform.SetTransform(lookAtMat);
	}

	void FPSCamera::walk(float distance) {
		this->Position += this->lookAt * distance;
	}
	void FPSCamera::strafe(float distance) {
		this->Position += this->bioAxis * distance;
	}

	void FPSCamera::rotateX(float angle) {
		phi = phi - angle;
		updateAxis();
	}

	void FPSCamera::rotateY(float angle) {
		theta = clamp(PI * .5, PI * -.5, theta + angle);
		updateAxis();
	}

	void FPSCamera::updateAxis() {
		float sin_phi = sin(phi), cos_phi = cos(phi);
		float sin_theta = sin(theta), cos_theta = cos(theta);

		lookAt = Vector3(cos_theta * sin_phi, sin_theta, cos_theta * cos_phi);

		bioAxis = normalize(cross(Vector3(0., 1., 0.), lookAt));
		up = normalize(cross(lookAt, bioAxis));
	}

	void FPSCamera::attach(SceneCamera* camera) {
		if (camera == nullptr) return;
		camera->transform.BindToWorld();

		Position = camera->transform.GetPosition();
		Mat4x4 Rotation = MatrixRotation(camera->transform.GetRotation());
		lookAt = normalize(Vector3(Rotation.a[0][0], Rotation.a[1][0], Rotation.a[2][0]));
		bioAxis = normalize(cross(Vector3(0., 1., 0.), lookAt));
		up = normalize(cross(lookAt, bioAxis));
		
		updateAngle();

		this->camera = camera;
	}

	void FPSCamera::look(Vector3 Position) {
		lookAt = normalize(Position - this->Position);
		bioAxis = normalize(cross(Vector3(0., 1., 0.), lookAt));
		up = normalize(cross(lookAt, bioAxis));

		updateAngle();
	}

	void FPSCamera::updateAngle() {
		float sintheta = lookAt[1], costheta = up[1],
			sinphi = -bioAxis[2], cosphi = bioAxis[0];

		this->theta = get_angle(sintheta, costheta);
		if (theta > PI) {
			this->theta -= PI * 2;
		}
		this->phi = get_angle(sinphi, cosphi);
	}
}