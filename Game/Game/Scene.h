#pragma once
#include <vector>
#include <memory>
#include "Mesh.h"
#include "uuid.h"
#include "Texture.h"
#include <string>
#include <map>
#include "Math.h"
#include "SceneCamera.h"
#include "SceneLight.h"
#include "SceneObject.h"
#include "SceneRootNode.h"


namespace Game {

	struct Scene;

	struct Scene {
		//these objects are all need to be loaded to GPU
		//
		std::map<std::string,Mesh> meshs;
		std::map<std::string,Texture> textures;
		//constant buffers for scene objects 
		//std::map<std::string,SceneBuffer> buffers;
		std::map<std::string,Material> materials;

		SceneRootNode* root = nullptr;
		SceneCamera* mainCamera = nullptr;
		SceneLight* mainLight = nullptr;

		std::string name;

		Scene(const Scene& other) = delete;
		const Scene& operator=(const Scene& other) = delete;

		Scene(const char* name);
		
		~Scene();

		Texture* skybox;
	};

	class SceneLoader {
	public:
		Scene* loadScene(const char* filename,bool useIndex = false);
		Scene* getScene(const char* sceneName);
		void destroyScene(const char* scene);
	private:
		std::map<std::string, Scene*> scenesMap;

		Scene* ObjPraser(Buffer& data,const char* name,bool useIndex);
		Scene* DaePraser(Buffer& data,const char* name,bool useIndex);
	};

	class SceneManager : public IRuntimeModule {
	public:
		bool initialize() override;
		void tick() override;
		void finalize() override;

		bool getScene(const char* sceneName,bool distroy_last_scene = false);
		bool loadScene(const char* filename,bool switch_to_scence = false,bool distroy_last_scene = false);

		SceneCamera* GetMainCamera() {
			if (currentScene) {
				return currentScene->mainCamera;
			}
			return nullptr;
		}
		SceneLight* GetMainLight() {
			if (currentScene) {
				return currentScene->mainLight;
			}
			return nullptr;
		}

		Scene* GetCurrentScene() { return currentScene; }

		SceneRootNode* QueryNode(std::string name,bool depth_first = true);
	private:
		void go_through_all_nodes(SceneRootNode* node,float deltaTime);
		void go_through_all_nodes(SceneRootNode* node,void (Game::SceneRootNode::*)());
		SceneRootNode* dfs_find_node(const std::string& name,SceneRootNode* node);
		SceneRootNode* bfs_find_node(const std::string& name);


		Scene* currentScene;
	};
}