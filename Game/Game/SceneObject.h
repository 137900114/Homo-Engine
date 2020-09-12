#pragma once
#include "SceneRootNode.h"
#include "Mesh.h"
#include "Material.h"

namespace Game {
	struct SceneObject : public SceneRootNode {
		SceneObject(const char* name, Scene* scene) :
			SceneRootNode(name, scene, OBJECT), geoMesh(nullptr){}

		Mesh* geoMesh;
	};

	
}