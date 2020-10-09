#include "PhyDataStructure.h"
#include "Memory.h"


namespace Game {
	extern MemoryModule gMemory;
}

using namespace Game;

BVHTree::BVHTree() {
	this->root = gMemory.New<BVHNode>();

}

void BVHTree::ClearAllNodes(BVHNode* root) {
	if (root->HasCollider()) {
		gMemory.Delete(root);
	}

	ClearAllNodes(root->GetChild(0));
	ClearAllNodes(root->GetChild(1));

	gMemory.Delete(root);
}


void BVHTree::Insert(PhyCollider* node) {
	if (root->GetCollider() == nullptr) {
		root->SetCollider(node);
		root->SetBound(node->GetBoundSphere());
	}
	else {
		InsertIntoNode(&root, node);
	}
}

//find the node which is closest to the node
void BVHTree::InsertIntoNode(BVHNode** root, PhyCollider* node) {
	if ((*root)->HasCollider()) {
		BVHNode* newRoot = gMemory.New<BVHNode>((*root)->GetParent());
		BVHNode* newChild = gMemory.New<BVHNode>(newRoot);
		(*root)->SetParent(newRoot);
		newRoot->SetChild(0, *root);
		newRoot->SetChild(1, newChild);

		newChild->SetCollider(node);
		newChild->SetBound(node->GetBoundSphere());

		BoundSphere newSphere = (*root)->GetBound().merge(node->GetBoundSphere());

		newRoot->SetBound(newSphere);
		*root = newRoot;

		return;
	}

	BVHNode* left = (*root)->GetChild(0);
	BVHNode* right = (*root)->GetChild(1);

	float ldis = length(left->GetBound().center - (*root)->GetBound().center);
	float rdis = length(right->GetBound().center - (*root)->GetBound().center);

	if (ldis < rdis) {
		InsertIntoNode(&left, node);
		(*root)->SetChild(0, left);
	}
	else {
		InsertIntoNode(&right, node);
		(*root)->SetChild(1, right);
	}

	(*root)->SetBound(left->GetBound().merge(right->GetBound()));

}

BVHNode* BVHTree::Find(PhyCollider* collider) {
	return FindNode(root,collider);
}

BVHNode* BVHTree::FindNode(BVHNode* node,PhyCollider* collider) {
	if (node->HasCollider()) {
		return node->GetCollider() == collider ? node : nullptr;
	}

	BVHNode* result = FindNode(node->GetChild(0), collider);
	if (result != nullptr) return result;
	result = FindNode(node->GetChild(1), collider);

	return result;
}

bool BVHTree::Remove(PhyCollider* collider) {
	return RemoveNode(root, collider);
}

bool BVHTree::RemoveNode(BVHNode* node,PhyCollider* collider) {
	if (node->HasCollider()) {
		if (node->GetCollider() == collider) {
			
			BVHNode* parent = node->GetParent();
			BVHNode* sibling = parent->GetSibling(node);

			BVHNode* grand = parent->GetParent();
			int parentIndex = grand->GetIndex(parent);

			grand->SetChild(parentIndex, sibling);

			gMemory.Delete(parent);
			gMemory.Delete(node);

			return true;
		}
		return false;
	}

	if (RemoveNode(node->GetChild(0), collider)
			|| RemoveNode(node->GetChild(1),collider)) {
		node->SetBound(node->GetChild(0)->GetBound().merge(node->GetChild(1)->GetBound()));
		return true;
	}
	return false;
}

void BVHTree::DetectIntersect(std::vector<PotentialIntersect>& inters) {
	if (root->GetChild(0) == nullptr || root->GetChild(1) == nullptr) {
		return;
	}
	DetectIntersectInNodes(inters, root->GetChild(0), root->GetChild(1));
}

void BVHTree::DetectIntersectInNodes(std::vector<PotentialIntersect>& inters, BVHNode* lhs, BVHNode* rhs) {
	BoundSphere lhsBound = lhs->GetBound(), rhsBound = rhs->GetBound();
	float distance = length(lhsBound.center - rhsBound.center);
	if (distance > lhsBound.radius + rhsBound.radius) {
		return;
	}

	if (rhs->HasCollider() && lhs->HasCollider()) {
		inters.push_back({ rhs->GetCollider(),lhs->GetCollider() });
	}

	if (rhs->HasCollider() ||
		(!lhs->HasCollider() && lhsBound.radius > rhsBound.radius)) {
		
		DetectIntersectInNodes(inters, lhs->GetChild(0), rhs);
		DetectIntersectInNodes(inters, lhs->GetChild(1), rhs);
	}
	else {
		DetectIntersectInNodes(inters, lhs, rhs->GetChild(0));
		DetectIntersectInNodes(inters, lhs, rhs->GetChild(1));
	}
}

void BVHTree::UpdateNode(BVHNode* node) {
	if (node->HasCollider()) {
		BoundSphere newSphere = node->GetCollider()->GetBoundSphere();
		node->SetBound(newSphere);
		return;
	}

	UpdateNode(node->GetChild(0));
	UpdateNode(node->GetChild(1));

	BoundSphere left = node->GetChild(0)->GetBound();
	BoundSphere right = node->GetChild(1)->GetBound();

	node->SetBound(left.merge(right));
}
