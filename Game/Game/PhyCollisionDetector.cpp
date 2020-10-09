#include "PhyCollisionDetector.h"

using namespace Game;

void PhyCollisionDetector::Register(PhyCollider* collider) {
	tree.Insert(collider);
}

void PhyCollisionDetector::UnRegister(PhyCollider* collider) {
	tree.Remove(collider);
}

void PhyCollisionDetector::update() {
	potentialIntersectList.clear();
	collisions.clear();
	
	tree.Update();
	tree.DetectIntersect(potentialIntersectList);

	PhyCollision collision;
	for (auto item : potentialIntersectList) {
		if (item.colliders[0]->GetRigidBody() == nullptr && item.colliders[1]->GetRigidBody() == nullptr)
			continue;
		bool collided = item.colliders[0]->DetectCollision(item.colliders[1],&collision);
		if (!collided) {
			collisions.push_back(collision);
		}
	}
}