#pragma once
#include "Vector.h"

namespace Game {
	class Random {
	public:
		static float rand();
		static Vector2 rand2();
		static Vector3 rand3();
		static Vector4 rand4();
	private:
		static int seed;
	};
}