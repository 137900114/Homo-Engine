#pragma once
#include "Math.h"


namespace Game {

	class PhyPartical {
	public:

		PhyPartical(float mass, float bouncing = .8f,Vector3 Position = Vector3(), Vector3 Vocel = Vector3()) :
			Position(Position), Vocel(Vocel),Acceleration(),Impact(),bouncing(bouncing) {
			SetMass(mass);
		}

		void Intergrate(float deltatime);

		inline void AddForce(Vector3 force) {
			Acceleration += force * invMass;
		}
			
		inline void AddImpact(Vector3 force) {
			Impact += force;
		}

		inline void SetPosition(Vector3 Position){
			this->Position =  Position;
		}

		inline const Vector3& GetPosition() const {
			return Position;
		}

		inline void SetVocel(Vector3 vocel) {
			this->Vocel = vocel;
		}

		inline const Vector3& GetVocel() const {
			return Vocel;
		}
			

		inline float GetMass() const{
			return mass;
		}

		inline float GetInvMass() const{
			return invMass;
		}

		inline void SetMass(float Mass) {
			if (Mass <= 0.) {
				this->mass = Mass;
				this->invMass = 0.;
			}
			else {
				this->mass = Mass;
				this->invMass = 1.f / Mass;
			}
		}

		inline bool isMassInfinitive() {
			return this->mass <= 0.f;
		}


		inline float GetBouncing() { return bouncing; }
		inline void SetBouncing(float bouncing) { this->bouncing = bouncing; }

	private:
		Vector3 Position;
		Vector3 Vocel;

		float mass;
		float invMass;
		float bouncing;

		Vector3 Acceleration;
		Vector3 Impact;
	};
	

}
