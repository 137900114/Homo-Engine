#pragma once
#include "SceneCamera.h"
#include "Timer.h"
#include "InputBuffer.h"


namespace Game {
	extern Timer gTimer;
	extern InputBuffer gInput;
}

void Game::SceneCamera::UpdateCameraBufferView() {
	Mat4x4 position = MatrixPosition(transform.Position);
	Mat4x4 rotation = MatrixRotation(transform.Rotation);

	cameraBufferPtr->invView = mul(position, rotation);
	cameraBufferPtr->view = cameraBufferPtr->invView.R();

	Mat4x4 view = cameraBufferPtr->view;

	cameraBufferPtr->view = view.T();
	Mat4x4 invView = cameraBufferPtr->invView;

	cameraBufferPtr->invView = invView.T();

	cameraBufferPtr->viewProj = mul( cameraBufferPtr->view, cameraBufferPtr->proj);
	cameraBufferPtr->invViewProj = mul(cameraBufferPtr->invProj,cameraBufferPtr->invView);

	
}

void Game::SceneCamera::UpdateCameraBufferProjection() {
	cameraBufferPtr->proj = camera.GetProj().T();
	cameraBufferPtr->invProj = camera.GetInvProj().T();

	cameraBufferPtr->nearPlane = camera.GetNearPlane();
	cameraBufferPtr->farPlane = camera.GetFarPlane();

	UpdateCameraBufferView();
}


Game::CBuffer* Game::SceneCamera::GetCameraCBuffer() {
	cameraBufferPtr->cameraPosition = transform.Position;
	cameraBufferPtr->timeLine = Game::gTimer.TotalTime();
	

	UpdateCameraBufferView() ;

	cameraBuffer.updated = true;

	return &cameraBuffer;
}

void Game::SceneCamera::initialize() {

	UpdateCameraBufferProjection();
	SceneRootNode::initialize();
}