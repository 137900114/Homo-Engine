#pragma once
#include <vector>
#include <string>
#include "Math.h"
#include "SceneComponent.h"
#include "SceneTransform.h"

namespace Game {

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
			initialized = true;
		}

		virtual void finalize() {
			for (int i = 0; i != components.size(); i++) {
				components[i]->finalize();
			}
		}

		void addComponent(SceneComponent* component) {
			component->initialize();
			components.push_back(component);
		}

		bool isInitialized() { return initialized; }

		SceneTransform transform;
		std::vector<SceneRootNode*> childs;
		std::vector<SceneComponent *> components;
		SceneNodeType type;
		std::string name;
		Scene* scene;

		bool initialized = false;
	};
}
