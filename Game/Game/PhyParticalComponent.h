#pragma once
#include "PhyPartical.h"
#include "SceneComponent.h"
#include "PFGenerator.h"

namespace Game {
	class PhyParticalComponent : public SceneComponent {
	public:
		virtual size_t componentSize() {
			return sizeof(PhyParticalComponent);
		}

		virtual void update(float deltatime) override;
		virtual void initialize() override;
		virtual void finalize() override;

		PhyParticalComponent(SceneRootNode* node, float mass = 1.f);

	private:
		PhyPartical partical;
		PFGravity* gravity;

		Vector3 lastPosition;
	};

}