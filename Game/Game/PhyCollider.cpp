#include "PhyCollider.h"

using namespace Game;

//normal direction should be from lhs's center to rhs's center
bool DetectCollision(PhyColliderSphere* lhs,PhyColliderSphere* rhs,PhyCollision* collision) {

	BoundSphere lsphere = lhs->GetBoundSphere();
	BoundSphere rsphere = rhs->GetBoundSphere();

	float distance = length(lsphere.center - rsphere.center);
	if (distance < lsphere.radius + rsphere.radius) {
		collision->collider[0] = lhs;
		collision->collider[1] = rhs;
		collision->depth = lsphere.radius + rsphere.radius - distance;
		collision->normal = normalize(lsphere.center - rsphere.center);
		collision->position = lerp(lsphere.center, rsphere.center, rsphere.radius / (lsphere.radius + rsphere.radius));

		return true;
	}

	return false;
}

bool DetectCollision(PhyColliderSphere* lhs,PhyColliderPlane* rhs,PhyCollision* collision){
	BoundSphere bound = lhs->GetBoundSphere();
	float sphereRadius = bound.radius;
	Vector3 sphereCenter = bound.center;
	Vector2 planeSize = rhs->GetSize();
	Vector4 planeSpaceCenter = mul(rhs->GetInvTransform(),Vector4(sphereCenter,1.f));

	//TODO: try to detect the edge collision between the plane and sphere
	if (::abs(planeSpaceCenter.y) > sphereRadius ||
		 ::abs(planeSpaceCenter.x) > planeSize.x ||
			::abs(planeSpaceCenter.z) > planeSize.y) {
		return false;
	}

	collision->collider[0] = rhs;
	collision->collider[1] = lhs;

	collision->depth = ::abs(sphereRadius - planeSpaceCenter.y);
	collision->normal = planeSpaceCenter.y > 0.f? rhs->GetNormal() : Vector3() - rhs->GetNormal();
	
	planeSpaceCenter.y = 0;
	collision->position = Vector3(mul(rhs->GetTransform(), planeSpaceCenter));
	
	return true;
}

BoundSphere PhyColliderSphere::GetBoundSphere() {
	Position = rigid->GetPosition();

	BoundSphere sphere;
	sphere.center = Position;
	sphere.radius = radius;

	return sphere;
}

bool PhyColliderSphere::DetectCollision(PhyCollider* col1, PhyCollision* collision) {
	
	switch (col1->GetColliderType()) {
	case PhyColliderType::COLLIDER_SPHERE:
		return ::DetectCollision(dynamic_cast<PhyColliderSphere*>(col1), this, collision);
	case PhyColliderType::COLLIDER_PLANE:
		return ::DetectCollision(this, dynamic_cast<PhyColliderPlane*>(col1), collision);
	}

	return false;
}

BoundSphere PhyColliderPlane::GetBoundSphere() {
	BoundSphere sphere;
	sphere.radius = length(size);
	sphere.center = o;

	return sphere;
}

bool PhyColliderPlane::DetectCollision(PhyCollider* col1,PhyCollision* collision) {
	switch (col1->GetColliderType()) {
	case PhyColliderType::COLLIDER_SPHERE:
		return ::DetectCollision(dynamic_cast<PhyColliderSphere*>(col1), this, collision);
	}
	return false;
}