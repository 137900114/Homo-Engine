#pragma once
#include "Mesh.h"

namespace Game {

	enum GeometryPlaneAxis {
		X_AXIS,Y_AXIS,Z_AXIS
	};

	class GeometryGenerator {
	public:

		//cube only generate position and normal data
		static Mesh generateCube(bool anitclockwise,Vector3 scaling);
		inline static Mesh generateCube(bool anticlockwise = true,float scaling = 1.f) {
			return std::move(generateCube(anticlockwise,Vector3(scaling, scaling, scaling)));
		}

		//generate position and normal data
		static Mesh generateSphere(bool anticlockwise,float radius = 1.f,uint32_t faceNum = 12);

		static Mesh generatePlane(uint32_t rowNum,uint32_t columNum,GeometryPlaneAxis axis = Z_AXIS,Vector2 halfSize = Vector2(1.,1.), bool anitclockwise = true);
	};

}