#pragma once
#include "Math.h"

#define MAX_LIGHT_PASS_NUM 1
namespace Game {
	//The z value of the position specifies the order of the drawing
	struct Vertex2D {
		Vector3 Position;
		Vector4 Color;
	};

	//the constant buffer will be aligned by 4 float data
	struct LightPass {
		struct {
			Vector3 lightDirection;
			float paddle;
			Vector4 lightIntensity;
		} Light[MAX_LIGHT_PASS_NUM];

		Vector3 ambient;
	};

	struct CameraPass {
		Mat4x4 projection;
		Mat4x4 invProjection;
		float timeLine;
	};

	struct ObjectPass {
		Mat4x4 world;
		Mat4x4 transInvWorld;
	};

	struct ImageVertex2D{
		Vector3 Position;
		Vector2 Texcoord;
	};
}