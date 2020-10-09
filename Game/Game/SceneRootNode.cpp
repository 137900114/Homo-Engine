#include "SceneRootNode.h"
#include "Memory.h"
#include "SceneObject.h"
#include "SceneCamera.h"
#include "SceneLight.h"

namespace Game {
	extern MemoryModule gMemory;

}

Game::SceneRootNode::~SceneRootNode() {
	for (int i = 0; i != childs.size(); i++) {
		switch (childs[i]->type) {
		case OBJECT:
			gMemory.Delete(reinterpret_cast<SceneObject*>(childs[i]));
			break;
		case NODE:
			gMemory.Delete(childs[i]);
			break;
		case CAMERA:
			gMemory.Delete(reinterpret_cast<SceneCamera*>(childs[i]));
		case LIGHT:
			gMemory.Delete(reinterpret_cast<SceneLight*>(childs[i]));
		default:
			break;
		}
	}

	//the compoents will be created from memory in memory pool
	//to deallocate the memory we have to manually get the size of the component
	//call the disconstruct function and deallocate the memory 
	for (int i = 0; i != components.size(); i++) {
		size_t componentSize = components[i]->componentSize();
		components[i]->~SceneComponent();
		gMemory.deallocate(componentSize,components[i]);
	}
	
}