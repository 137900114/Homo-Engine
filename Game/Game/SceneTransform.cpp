#include "SceneTransform.h"

using namespace Game;


void SceneTransform::UnpackTransform(Mat4x4 transform) {
	Game::UnpackTransfrom(transform, Position, Rotation, Scale);
	this->relativeWorld = transform;
	this->RootWorld = Mat4x4::I();

	WorldPosition = Position;
	WorldRotation = Rotation;
	WorldScale = Scale;

	type = TransformType::WORLD;
}

void SceneTransform::UnpackTransform(Mat4x4 transform,Mat4x4 root) {
	this->relativeWorld = mul(root.R(), transform);

	Game::UnpackTransfrom(this->relativeWorld, Position, Rotation, Scale);
	this->RootWorld = root;

	Game::UnpackTransfrom(GetWorld(), WorldPosition, WorldRotation, WorldScale);
	type = TransformType::BIND_TO_ROOT;
}

void SceneTransform::SetTransform(Vector3 Position,Vector3 Rotation,Vector3 Scale) {
	this->Position = Position;
	this->Rotation = Rotation;
	this->Scale = Scale;

	this->relativeWorld = PackTransfrom(Position, Rotation, Scale);

	if (type == TransformType::BIND_TO_ROOT) {
		Mat4x4 world = GetWorld();
		Game::UnpackTransfrom(world, WorldPosition, WorldRotation, WorldScale);
	}
	else if(type == TransformType::WORLD){
		WorldPosition = Position;
		WorldRotation = Rotation;
		WorldScale = Scale;
	}
}

void SceneTransform::SetTransform(Mat4x4 trans) {
	this->relativeWorld = trans;
	Game::UnpackTransfrom(trans, Position, Rotation, Scale);
	if (type == TransformType::BIND_TO_ROOT) {
		Mat4x4 world = GetWorld();
		Game::UnpackTransfrom(world, WorldPosition, WorldRotation, WorldScale);
	}
	else if (type == TransformType::WORLD) {
		WorldPosition = Position;
		WorldRotation = Rotation;
		WorldScale = Scale;
	}
}

void SceneTransform::BindOnRoot(Mat4x4 RootWorld) {
	if (type == TransformType::WORLD)
		type = TransformType::BIND_TO_ROOT;
	Mat4x4 world = GetWorld();
	this->relativeWorld = mul(RootWorld.R(), world);
	
	Game::UnpackTransfrom(this->relativeWorld, Position, Rotation, Scale);
}


void SceneTransform::BindToWorld() {
	if (type == TransformType::BIND_TO_ROOT) {
		this->relativeWorld = GetWorld();
		this->RootWorld = Mat4x4::I();
		Game::UnpackTransfrom(this->relativeWorld,Position,Rotation,Scale);
	}
}


void SceneTransform::SetRootWorld(Mat4x4 RootWorld) {
	if (type != TransformType::WORLD) {
		this->RootWorld = RootWorld;

		Mat4x4 world = GetWorld();
		Game::UnpackTransfrom(world, WorldPosition, WorldRotation, WorldScale);
	}
}