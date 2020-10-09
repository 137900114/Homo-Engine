#include "PhysicsSimulater.h"
#include "Timer.h"

namespace Game {
	extern Timer gTimer;

	extern PhysicsSimulater gPhysicsSimulater;
}

using namespace Game;

bool PhysicsSimulater::initialize() {
	return true;
}

void PhysicsSimulater::tick() {

	float deltatime = gTimer.DeltaTime();
	pfRegister.tick(deltatime);

	for (auto& item : particals) {
		item->Intergrate(deltatime);
	}
}

void PhysicsSimulater::finalize() {

}


void PhysicsSimulater::RegisterPartical(PhyPartical* partical) {
	for (int i = 0; i != particals.size(); i++) {
		if (particals[i] == partical) return;
	}

	particals.push_back(partical);
}

void PhysicsSimulater::RegisterParticalForce(PhyPartical* particals,PFGenerator* pfg) {
	this->pfRegister.Register(particals, pfg);
}

void PhysicsSimulater::UnRegisterParticalForce(PhyPartical* particals,PFGenerator* pfg) {
	this->pfRegister.UnRegister(particals, pfg);
}

void PhysicsSimulater::UnRegister(PhyPartical* partical) {
	for (auto iter = particals.begin(); iter != particals.end(); iter++) {
		if (*iter == partical) {
			particals.erase(iter);
			return;
		}
	}
} 
