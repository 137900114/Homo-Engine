#pragma once
#include <vector>
#include <memory>
#include "Mesh.h"
#include "uuid.h"
#include "Image.h"
#include <string>
#include <map>


namespace Game {

	struct Scene;

	enum SceneNodeType {
		Root,Object,Material
	};

	struct SceneNode {
		SceneNodeType type;
		std::string name;
		Scene* scene;

		SceneNode(const char* name,SceneNodeType type,Scene* scene):
		type(type),name(name),scene(scene)
		{}
	};

	//The buffer need to upload to gpu has UUID.
	struct SceneBuffer {
		UUID   gpuDataToken;
		Buffer buffer;
		bool   updated = false;
	};

	struct SceneRootNode : public SceneNode{
		SceneRootNode(Scene* scene):
		SceneNode("root",Root,scene){ }

		std::vector<SceneNode*> childs;
	};


	struct SceneMaterial {

	};

	struct Scene {
		//these objects are all need to be loaded to GPU
		//
		std::map<std::string,Mesh> meshs;
		std::map<std::string,Image> images;
		//constant buffers for scene objects 
		std::map<std::string,SceneBuffer> buffers;
		std::map<std::string,SceneMaterial> materials;

		SceneRootNode* root;
		std::string name;

		Scene(const Scene& other) = delete;
		const Scene& operator=(const Scene& other) = delete;

		Scene(const char* name);

		~Scene();
	};

	class SceneLoader {
	public:
		Scene* loadScene(const char* filename);
		Scene* getScene(const char* sceneName);
		void destroyScene(const char* scene);
	private:
		std::map<std::string, Scene*> scenesMap;

		Scene* ObjPraser(Buffer& data,const char* name);
	};
}