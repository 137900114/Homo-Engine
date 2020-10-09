#pragma once
#include "Math.h"
#include "IRuntimeModule.hpp"
#include "PFGenerator.h"
#include "PhyPartical.h"
#include <vector>


namespace Game {
	class PhysicsSimulater : public IRuntimeModule {
	public:
		virtual bool initialize() override;
		virtual void tick() override;
		virtual void finalize() override;

		void RegisterPartical(PhyPartical* partical);

		void RegisterParticalForce(PhyPartical* partical, PFGenerator* pfg);
		void UnRegisterParticalForce(PhyPartical* partical, PFGenerator* pfg);

		void UnRegister(Game::PhyPartical* partical);

	private:
		PFRegister pfRegister;
		std::vector<PhyPartical*> particals;
	};
}