#pragma once
#include "PhyBounding.h"
#include "PhyCollider.h"
#include <vector>

namespace Game {

	struct PotentialIntersect {
		PhyCollider* colliders[2];
	};

	
	class BVHNode {
	private:
		BoundSphere bound;
		BVHNode* children[2] = {nullptr,nullptr};
		BVHNode* parent;

		PhyCollider* collider;
	public:

		bool HasCollider() { return collider != nullptr; }
		BVHNode() {
			bound.center = Vector3();
			bound.radius = 0.f;
			this->collider = nullptr;
			this->parent = nullptr;
		}

		BVHNode(BVHNode* parent) {
			collider = nullptr;
			bound.center = Vector3();
			bound.radius = 0.f;
			this->parent = parent;
		}

		BVHNode(BVHNode* parent,PhyCollider* collider) {
			this->collider = collider;
			bound = collider->GetBoundSphere();
			this->parent = parent;
		}

		inline BVHNode* GetChild(int index) { return children[index]; }
		inline void SetChild(int index, BVHNode* node) {
			children[index] = node;
		}
		
		inline BoundSphere GetBound() {return bound;}
		inline void SetBound(BoundSphere sphere) { this->bound = sphere; }

		inline PhyCollider* GetCollider() { return collider; }
		inline void SetCollider(PhyCollider* collider) { this->collider = collider; }

		inline BVHNode* GetParent() { return parent; }
		inline void SetParent(BVHNode* parent) { this->parent = parent; }

		inline BVHNode* GetSibling(BVHNode* node) {
			if (node == children[0]) return children[1];
			else if (node == children[1]) return children[0];
			else return nullptr;
		}

		inline int GetIndex(BVHNode* node) {
			if (node == children[0]) return 0;
			else if (node == children[1]) return 1;
			return -1;
		}
	};


	class BVHTree {
	public:
		BVHTree();
		~BVHTree() { ClearAllNodes(root); }

		void Insert(PhyCollider* collider);

		BVHNode* Find(PhyCollider* collider);

		bool Remove(PhyCollider* collider);

		void DetectIntersect(std::vector<PotentialIntersect>& inters);

		// update the bounds
		inline void Update() { UpdateNode(root); }

		BVHNode* GetRoot() { return root; }
	private:
		void InsertIntoNode(BVHNode** root,PhyCollider* collider);
		BVHNode* FindNode(BVHNode* node,PhyCollider* collider);
		bool RemoveNode(BVHNode* node,PhyCollider* collider);

		void DetectIntersectInNodes(std::vector<PotentialIntersect>& inters,BVHNode* rhs,BVHNode* lhs);
		void UpdateNode(BVHNode* node);

		void ClearAllNodes(BVHNode* root);
		BVHNode* root;

	};


}
