#pragma once
#include "Math.h"
#include "PhyBounding.h"
#include "PhyRigidBody.h"
//collider discribes a geometrie's shape when the other object want to interact with it
namespace Game {

	enum class PhyColliderType {
		COLLIDER_SPHERE,
		COLLIDER_CUBE,
		COLLIDER_PLANE,
		//COLLIDER_PLOYGON for some complex object use ploygon discribe it's shape
	};

	struct PhyCollision;

	class PhyCollider {
	public:
		virtual BoundSphere GetBoundSphere() = 0;
		virtual bool DetectCollision(PhyCollider* other,PhyCollision* result) = 0;

		inline bool IsColliderRigidBody() {
			return rigid != nullptr;
		}

		inline PhyRigidBody* GetRigidBody() {
			return rigid;
		}

		PhyColliderType GetColliderType() {
			return type;
		}

		PhyCollider(PhyColliderType type,PhyRigidBody* rigid = nullptr) {
			this->type = type;
			this->rigid = rigid;
		}
	protected:
		PhyColliderType type;
		PhyRigidBody* rigid;
	};
	
	struct PhyCollision {
		PhyCollider* collider[2];
		Vector3 position;
		Vector3 normal;
		Vector3 depth;
	};

	class PhyColliderSphere : public PhyCollider{
	public:
		PhyColliderSphere(Vector3 Position,float radius,PhyRigidBody* rigid = nullptr) :
			PhyCollider(PhyColliderType::COLLIDER_SPHERE,rigid){
			this->Position = Position;
			this->radius = radius;
		}

		virtual BoundSphere GetBoundSphere() override;
		virtual bool DetectCollision(PhyCollider* col1,PhyCollision* result) override;
	private:
		Vector3 Position;
		float radius;
	};

	class PhyColliderPlane: public PhyCollider{
	public:
		PhyColliderPlane(Vector3 o,Vector2 size,Vector3 normal,Vector3 _tangent):
		PhyCollider(PhyColliderType::COLLIDER_PLANE,nullptr){
			this->normal = normalize(normal);
			Vector3 biotan = normalize(cross(_tangent,normal));
			Vector3 tangent = cross(biotan, normal);

			Transform = Mat4x4(tangent.x,normal.x,biotan.x,o.x,
							   tangent.y,normal.y,biotan.y,o.y,
							   tangent.z,normal.z,biotan.z,o.z,
							   0	   ,0        ,0       ,1.f);

			invTransform = Transform.R();

			this->o = o;
			this->size = size;
		}

		virtual BoundSphere GetBoundSphere() override;
		virtual bool DetectCollision(PhyCollider* col1,PhyCollision* result) override;

		inline Vector3 GetNormal() { return normal; }
		inline Vector3 GetOriginal() { return o; }
		inline Vector2 GetSize() { return size; }
		inline Mat4x4 GetInvTransform() { return invTransform; }
		inline Mat4x4 GetTransform() { return Transform; }
	private:
		Vector3 normal;
		Mat4x4 invTransform;
		Mat4x4 Transform;
		Vector3 o;
		Vector2 size;
	};
}
