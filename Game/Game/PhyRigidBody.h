#pragma once
#include "Math.h"

namespace Game {
	class PhyRigidBody {
	public:
		
		void intergrate(float deltatime);


		inline Vector3 GetPosition() {
			return position;
		}

		inline Vector3 GetVelocity() {
			return velocity;
		}

		inline float GetMass() {
			return mass;
		}

		inline float GetInvMass() {
			return invMass;
		}


		inline void ClearForceAccum() {
			forceAccum = Vector3();
			torqueAccum = Vector3();
			angleImpulse = Vector3();
			Impulse = Vector3();
		}

		void SetMass(float mass) {
			this->mass = mass;
			this->invMass = 1.f / mass;
		}
		void SetInertiaTensor(const Mat3x3& inertiaTensor) {
			this->Inertia = inertiaTensor;
			this->invInertia = this->Inertia.R();
		}

		void SetVelocity(Vector3 v) {
			this->velocity = v;
		}

		void SetPosition(Vector3 position) {
			this->position = position;
		}
		
	private:
		inline Mat3x3 GetWorldInertia(const Mat4x4& transform, const Mat3x3& Inertia) {
			Mat3x3 rotation(transform);
			return mul(mul(rotation,Inertia),rotation.T());
		}
		Mat3x3 GetWorldInvInertia(const Mat4x4& transform, const Mat3x3& invInertia) {
			Mat3x3 rotation(transform);
			return mul(mul(rotation,invInertia),rotation.T());
		}

		Mat4x4 transform;
		Vector3 velocity;

		Vector3 angleSpeed;
		Quaternion rotation;// = the rotation axis * angle 

		Vector3 position;
		
		float invMass;
		float mass;

		Vector3 forceAccum;
		Vector3 torqueAccum;
		Vector3 Impulse;
		Vector3 angleImpulse;

		Mat3x3 Inertia;
		Mat3x3 invInertia;
	};
}