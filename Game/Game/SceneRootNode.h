#pragma once
#include <vector>
#include <string>
#include "Math.h"
#include "SceneComponent.h"

namespace Game {

	struct SceneTransform {
		Vector3 Position;
		Vector3 Rotation;
		Vector3 Scale;
		Mat4x4  World;
	};

	enum SceneNodeType {
		NODE, OBJECT, CAMERA, LIGHT
	};

	struct Scene;


	struct SceneRootNode {
		SceneRootNode(const char* name, Scene* scene, SceneNodeType type = NODE) :
			scene(scene), type(type), name(name) { }
		virtual ~SceneRootNode();

		virtual void update(float deltatime) {
			for (int i = 0; i != components.size(); i++) {
				components[i]->update(deltatime);
			}
		}

		virtual void initialize() {
			for (int i = 0; i != components.size(); i++) {
				components[i]->initialize();
			}
		}

		virtual void finalize() {
			for (int i = 0; i != components.size(); i++) {
				components[i]->finalize();
			}
		}

		SceneTransform transform;
		std::vector<SceneRootNode*> childs;
		std::vector<SceneComponent *> components;
		SceneNodeType type;
		std::string name;
		Scene* scene;
	};
}
