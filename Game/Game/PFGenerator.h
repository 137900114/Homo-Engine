#pragma once
#include "PhyPartical.h"
#include <vector>


namespace Game {
	class PFGenerator {
	public:
		PFGenerator() { }

		virtual void UpdateForce(PhyPartical* partical,float deltatime) = 0;
	private:

	};


	class PFRegister {
	private:
		struct PFRegistration{
			PhyPartical* PhyPartical;
			PFGenerator* Force;
		};
			
		std::vector<PFRegistration> pForces;

	public:
		void tick(float deltatime);

		void Register(PhyPartical* partical,PFGenerator* force);
		bool UnRegister(PhyPartical* partical,PFGenerator* force);
	};

	class PFGravity : public PFGenerator{
	public:
		PFGravity(float gravityScale = 1.f,bool activated = true):
			activated(activated){
			this->gravityScale = gravityScale;
		}

		bool Activated() { return activated; }
		void SetActive(bool activated) { this->activated = activated; }
		float GetGravityScale() { return gravityScale; }
		void SetGravityScale(float gravityScale) { 
			if(gravityScale > 0)
				this->gravityScale = gravityScale;
		}

		virtual void UpdateForce(PhyPartical* partical,float deltatime) override;
			
		static PFGravity DefaultGravity;
	private:
		float gravityScale;
		bool activated;
	};
		

	//usually friction forces will be effected by objects's vocelity and vocelity's length's squre
	class PFFriction : public PFGenerator{
	public:
		PFFriction(float k1, float k2) {
			this->k1 = k1, this->k2 = k2;
		}

		virtual void UpdateForce(PhyPartical* partical, float deltatime) override;

	private:
		float k1, k2;
	};

	class PFSpring : public PFGenerator {
	public:
		PFSpring(PhyPartical* partical,PhyPartical* other,float springConstant) {
			this->other = other;
			originLength = length(partical->GetPosition() - other->GetPosition());
			this->springConstant = springConstant;
		}

		PFSpring(float originalLength,PhyPartical* other,float springConstant) {
			this->other = other;
			this->originLength = originalLength;
			this->springConstant = springConstant;
		}

		virtual void UpdateForce(PhyPartical* partical,float deltatime) override;

	private:
		PhyPartical* other;

		float originLength;
		float springConstant;
	};

	
	class PFAnchoredSpring : public PFGenerator{
	public:
		PFAnchoredSpring(PhyPartical* partical,Vector3 origin,float springConstant) {
			this->originLength = length(partical->GetPosition() - origin);
			this->springConstant = springConstant;

			this->anchor = origin;
		}

		PFAnchoredSpring(float originLength,Vector3 origin,float springConstant) {
			this->originLength = originLength;
			this->springConstant = springConstant;

			this->anchor = origin;
		}

		inline void SetAnchor(Vector3 anchor) { this->anchor = anchor; }
		inline Vector3 GetAnchor() { return anchor; }

		virtual void UpdateForce(PhyPartical* partical,float deltatime) override;

	private:
		Vector3 anchor;
		
		float originLength;
		float springConstant;
	};

	class PFHardAnchoredSpring : public PFGenerator {
	public:
		PFHardAnchoredSpring(PhyPartical* partical, Vector3 origin, float springConstant,float damping) {
			this->springConstant = springConstant;

			this->anchor = origin;
			this->damping = damping;

			gamma = 4 * springConstant - damping * damping;
			if (gamma >= 0) gamma = sqrt(gamma) * .5f;
			invGamma = gamma;
		}

		inline void SetAnchor(Vector3 anchor) { this->anchor = anchor; }
		inline Vector3 GetAnchor() { return anchor; }

		virtual void UpdateForce(PhyPartical* partical, float deltatime) override;

	private:
		Vector3 anchor;

		float springConstant;
		float damping;

		float gamma;
		float invGamma;
	};

}