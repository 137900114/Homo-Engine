#pragma once
#include "Math.h"
#include "SceneRootNode.h"

namespace Game {
	struct SceneLight : public SceneRootNode {
		SceneLight(const char* name, Scene* scene, Light& light) :
			SceneRootNode(name, scene, LIGHT) {
			lightCBuffer.regID = 2;
			lightCBuffer.updated = true;
			lightCBuffer.data.resize(sizeof(Light));

			this->light = reinterpret_cast<Light*>(lightCBuffer.data.data);
			*this->light = light;

			Vector3 intensity = this->light->lightIntensity;
			for (int i = 0; i != 3; i++) {
				if (intensity.raw[i] > 1.) {
					intensity = normalize(intensity);
					break;
				}
			}
			this->light->lightIntensity = intensity;
		}


		CBuffer* GetLightCBuffer() { 
			if (light->lightType == LIGHT_TYPE_POINT) {
				light->lightVec = transform.Position;
			}
			else if (light->lightType == LIGHT_TYPE_DIRECTIONAL) {
				light->lightVec = Vector3(mul(MatrixRotation(transform.Rotation), Vector4(1., 0., 0.,0.)));
			}
			lightCBuffer.updated = true;
			return &lightCBuffer;
		}
		


		Light* light;
		CBuffer lightCBuffer;
	};
}