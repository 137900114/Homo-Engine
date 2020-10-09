#pragma once
#include "Math.h"

namespace Game {

	//
	struct BoundSphere {
		Vector3 center;
		float radius;

		BoundSphere merge(BoundSphere other) {
			float distance = length(center - other.center);
			float radius = (distance + this->radius + other.radius) * .5f;

			BoundSphere newSphere;
			if (radius > this->radius && radius > other.radius) {
				newSphere.radius = radius;

				newSphere.center = center * ((radius - this->radius) / distance)
					+ other.center * ((radius - other.radius) / distance);
			}
			else if (radius < this->radius) {
				newSphere = *this;
			}
			else if (radius < other.radius) {
				newSphere = other;
			}

			return newSphere;
		}
	};


}