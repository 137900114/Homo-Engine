#pragma once
#include "Math.h"


namespace Game {
	class SceneTransform {
	public:
		enum class TransformType {
			BIND_TO_ROOT,
			WORLD
		};

		void SetTransform(Mat4x4 trans);
		void SetTransform(Vector3 Position, Vector3 Rotation, Vector3 Scale);

		inline void SetPosition(Vector3 Position) { SetTransform(Position, this->Rotation, this->Scale); }
		inline void SetRotation(Vector3 Rotation) { SetTransform(this->Position, Rotation, this->Scale); }
		inline void SetScale(Vector3 Scale) { SetTransform(this->Position, this->Rotation, Scale); }

		inline Vector3 GetPosition() { return Position; }
		inline Vector3 GetRotation() { return Rotation; }
		inline Vector3 GetScale() { return Scale; }

		inline Vector3 GetWorldPosition() { return WorldPosition; }
		inline Vector3 GetWorldRotation() { return WorldRotation; }
		inline Vector3 GetWorldScale() { return WorldScale; }

		//the object's world position will not be changed,while the root of the object will change
		void BindOnRoot(Mat4x4 RootWorld);
		
		void SetRootWorld(Mat4x4 RootWorld);
		inline Mat4x4 GetRootWorld() { return RootWorld; }

		Mat4x4 GetWorld() { return mul(RootWorld, relativeWorld); }

		SceneTransform():Scale(Vector3(1.,1.,1.)),RootWorld(Mat4x4::I()),relativeWorld(Mat4x4::I()){
			type = TransformType::BIND_TO_ROOT;
		}

		void UnpackTransform(Mat4x4 transform);
		void UnpackTransform(Mat4x4 transform,Mat4x4 root);

		TransformType GetType() { return type; }
		void BindToWorld();
	private:
		Vector3 Position;
		Vector3 Rotation;
		Vector3 Scale;
		Mat4x4  relativeWorld;
		Mat4x4  RootWorld;

		Vector3 WorldPosition;
		Vector3 WorldRotation;
		Vector3 WorldScale;

		TransformType type;
	};



}