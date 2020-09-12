#pragma once
#include "Math.h"


namespace Game {
	struct ShaderObjectBuffer {
		Mat4x4 world;
		Mat4x4 invWorld;
	};

	struct ShaderStanderdCamera {
		Mat4x4 view;
		Mat4x4 invView;
		Mat4x4 proj;
		Mat4x4 invProj;
		Mat4x4 viewProj;
		Mat4x4 invViewProj;

		Vector3 cameraPosition;
		float timeLine;

		float nearPlane;
		float farPlane;
	};

	enum ShaderLightType{
		LIGHT_TYPE_POINT = 0,
		LIGHT_TYPE_DIRECTIONAL  = 1 
	};

	struct Light {
		int lightType;
		Vector3 lightVec;
		float lightAttuation;
		Vector3 lightIntensity;
	};
}
