#include "Camera.h"

#include "Timer.h"

namespace Game {
	//extern Timer gTimer;

	/*
	void Camera::UpdateViewMatrix() {
		float mat[] = {
			Right.x,Right.y,Right.z,-dot(Right,Position),
			   Up.x,   Up.y,   Up.z,-dot(   Up,Position),
			 Look.x, Look.y, Look.z,-dot( Look,Position),
			      0,      0,      0,                  1
		};

		buffer.view = Mat4x4(mat);
		buffer.invView = buffer.view.R();
		buffer.viewProj = mul(buffer.proj, buffer.view);
		buffer.invViewProj = mul(buffer.invView, buffer.invProj);
	}

	void Camera::LookAt(Vector3 Position,Vector3 Target,Vector3 Up) {
		this->Position = Position;
		Look = normalize(Target - Position);
		Right = normalize(cross(Look, Up));
		this->Up = normalize(cross(Right, Look));

		UpdateViewMatrix();
	}

	void Camera::Translate(Vector2 pos) {
		Position = Look * pos.x + Right * pos.y + Position;
		UpdateViewMatrix();
	}

	void Camera::Pitch(float angle) {
		Mat4x4 Rotation = MatrixRotation(Right, angle);

		Look = Vector3(mul(Rotation, Vector4(Look, 0.)));
		Up = Vector3(mul(Rotation, Vector4(Up, 0.)));
		Right = Vector3(mul(Rotation, Vector4(Right, 0.)));

		UpdateViewMatrix();
	}

	void Camera::RotateY(float angle) {
		Mat4x4 Rotation = MatrixRotation(Up, angle);

		Look = Vector3(mul(Rotation, Vector4(Look, 0.)));
		Up = Vector3(mul(Rotation, Vector4(Up, 0.)));
		Right = Vector3(mul(Rotation, Vector4(Right, 0.)));

		UpdateViewMatrix();
	}

	void Camera::Transform(Mat4x4 trans) {
		Position = mul(trans, Vector4(Position, 1.0f));
		Look = normalize(mul(trans, Vector4(Look, 0.f)));
		Right = normalize(mul(trans, Vector4(Right, 0.f)));
		Up = normalize(mul(trans, Vector4(Up, 0.f)));

		UpdateViewMatrix();		
	}*/

	void Camera::UpdateProjection() {
		proj = MatrixProjection(aspect, fovY, near, far);
		invProj = proj.R();
	}

	/*CameraBuffer Camera::GetCameraBuffer() {
		
		CameraBuffer result = buffer;
		result.farPlane = far;
		result.nearPlane = near;
		result.timeLine = gTimer.TotalTime();

		return result;
	}*/

}