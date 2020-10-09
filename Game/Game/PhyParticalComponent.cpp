#include "PhysicsSimulater.h"
#include "PhyParticalComponent.h"
#include "SceneRootNode.h"

namespace Game {
	extern Game::PhysicsSimulater gPhysicsSimulater;
}

using namespace Game;

PhyParticalComponent::PhyParticalComponent(SceneRootNode* node, float mass) :SceneComponent(node),
partical(mass) {
	this->gravity = &PFGravity::DefaultGravity;
}

void PhyParticalComponent::initialize() {
	gPhysicsSimulater.RegisterPartical(&this->partical);
	root->transform.BindToWorld();
	partical.SetPosition(root->transform.GetPosition());

	lastPosition = root->transform.GetPosition();
	gPhysicsSimulater.RegisterParticalForce(&this->partical, this->gravity);
}

void PhyParticalComponent::update(float /*deltatime*/) {
	Vector3 particalPosition = partical.GetPosition();
	Vector3 transformPosition = root->transform.GetPosition();

	if (lastPosition == transformPosition) {
		root->transform.SetPosition(particalPosition);
		lastPosition = particalPosition;
	}
	else {
		partical.SetPosition(transformPosition);
		lastPosition = transformPosition;
	}
}


void PhyParticalComponent::finalize() {
	root->transform.BindOnRoot(Mat4x4::I());

	gPhysicsSimulater.UnRegister(&partical);
	gPhysicsSimulater.UnRegisterParticalForce(&partical, this->gravity);
}