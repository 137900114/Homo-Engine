#pragma once
#include "PhyDataStructure.h"
#include "PhyCollider.h"

namespace Game {
	class PhyCollisionDetector {
	public:
		void Register(PhyCollider* collider);
		void UnRegister(PhyCollider* collider);

		void update();

		std::vector<PhyCollision>* GetCollisions() {
			return &collisions;
		}
	private:

		BVHTree tree;
		std::vector<PotentialIntersect> potentialIntersectList;
		std::vector<PhyCollision> collisions;
	};
}