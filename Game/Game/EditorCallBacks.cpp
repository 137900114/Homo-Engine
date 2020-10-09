#include "EditorCallBacks.h"
#include <map>
#include <string>
#include "EditorGUIModule.h"
#include "Reflect_Registry.h"

using namespace Game;

_SETUP_REFLECTION(EditorElementMenuItem::MenuItemCallBack)
_SETUP_REFLECTION(EditorElement::EditorElementCallBack)

#define DEFINE_MENU_ITEM_CALLBACK(Func) void _MENU_ITEM_##Func##(EditorElementMenuItem* item, EditorElementMenuBar* menuBar, EditorGUIWindow* window);\
_REFLECT_REGISTER(Func,EditorElementMenuItem::MenuItemCallBack,_MENU_ITEM_)\
void _MENU_ITEM_##Func##(EditorElementMenuItem* item, EditorElementMenuBar* menuBar, EditorGUIWindow* window)

#define DEFINE_ELEMENT_CALLBACK(Func) void _ELEMENT_##Func##(EditorElement* self);\
_REFLECT_REGISTER(Func,EditorElement::EditorElementCallBack,_ELEMENT_)\
void _ELEMENT_##Func##(EditorElement* self)


EditorElementMenuItem::MenuItemCallBack EditorCallBackManager::getMenuItemCallBack(const char* name) {
	if (name == nullptr) return nullptr;
	EditorElementMenuItem::MenuItemCallBack callback = nullptr;
	RefRegister<EditorElementMenuItem::MenuItemCallBack>::find(name,callback);
	return callback;
}


EditorElement::EditorElementCallBack EditorCallBackManager::getElementCallBack(const char* name) {
	if (name == nullptr) return nullptr;
	EditorElement::EditorElementCallBack result = nullptr;
	RefRegister<EditorElement::EditorElementCallBack>::find(name, result);
	return result;
}


#include "IApplication.h"
#include "Scene.h"
#include "GraphicModule.h"
#include "Texture.h"
#include "FPSCamera.h"

namespace Game {
	extern IApplication* app;
	extern SceneManager gSceneManager;
	extern EditorGUIModule gEditorGUIModule;
	extern GraphicModule* gGraphic;
	extern TextureLoader gImageLoader;

	extern FPSCamera fpsCamera;//defined in EditorElementPraser
}

#include "imgui/imgui.h"

DEFINE_MENU_ITEM_CALLBACK(OpenFile) {
	std::string path = app->getPath();
	if (path.empty()) return;

	if (!gSceneManager.loadScene(path.c_str(), true)) {
		std::string message = "fail to open file " + path;
		app->messageBox("Open", message.c_str());
		return;
	}


	//------------------------------------------------------//
	SceneCamera* camera = gSceneManager.GetMainCamera();

	float aspect = (float)app->getSysConfig().width / (float)app->getSysConfig().height;
	camera->SetAspect(aspect);

	SceneLight* light = gSceneManager.GetMainLight();
	light->transform.SetPosition(Vector3(0., 0., -1.));


	//--------------------------------------------------------//
	EditorGUIWindow* scene_window = gEditorGUIModule.getWindow("Scene");
	EditorGUIWindow* inspect_window = gEditorGUIModule.getWindow("Inspect");
	EditorGUIWindow* welcome_window = gEditorGUIModule.getWindow("welcome");
	if (scene_window) {
		scene_window->setEnabled(true);
	}
	if (welcome_window) {
		welcome_window->setEnabled(false);
	}
	if (inspect_window) {
		inspect_window->setEnabled(true);
	}

	Scene* scene = gSceneManager.GetCurrentScene();
	if (scene->skybox == nullptr) {
		CubeMapFileArray cube;
		cube.back = "DefaultSkyBox\\default_skybox_back.bmp";
		cube.front = "DefaultSkyBox\\default_skybox_front.bmp";
		cube.down = "DefaultSkyBox\\default_skybox_down.bmp";
		cube.left = "DefaultSkyBox\\default_skybox_left.bmp";
		cube.right = "DefaultSkyBox\\default_skybox_right.bmp";
		cube.up = "DefaultSkyBox\\default_skybox_up.bmp";
		scene->textures["_default_sky_box"] = std::move(gImageLoader.loadCubeMap(cube));

		scene->skybox = &scene->textures["_default_sky_box"];
	}
	gGraphic->skybox(*scene->skybox, scene->mainCamera);
}


DEFINE_MENU_ITEM_CALLBACK(Exit) {
	app->Quit();
}


DEFINE_MENU_ITEM_CALLBACK(Default) {
	std::string itemPath = item->getPath();
	std::string message = "no avaliable callback function for " + itemPath + ".This menu function has not been finished yet.";

	app->messageBox("no avaliable callback",message.c_str());
}


SceneRootNode* selectedNode = nullptr;
float a[3] = {114.,514.,1919.};
void SceneObjectTablePraseNode(SceneRootNode* root) {
	if (root == nullptr) return;

	for (auto item : root->childs) {
		if (ImGui::TreeNode(item->name.c_str())) {
			if (ImGui::SmallButton("select")) {
				selectedNode = item;
			}
			if (ImGui::SmallButton("look at")) {
				fpsCamera.look(item->transform.GetWorldPosition());
			}
			SceneObjectTablePraseNode(item);
			ImGui::TreePop();
		}
	}
}


DEFINE_ELEMENT_CALLBACK(SceneObjectTableUpdate) {
	Scene* currentScene = gSceneManager.GetCurrentScene();
	if (!currentScene) return;
	SceneRootNode* node = currentScene->root;
	SceneObjectTablePraseNode(node);
}

DEFINE_MENU_ITEM_CALLBACK(ShowSceneWindow) {
	EditorGUIWindow* scene = gEditorGUIModule.getWindow("Scene");
	if (scene != nullptr) {
		scene->setEnabled(true);
	}
}

DEFINE_MENU_ITEM_CALLBACK(ShowInspectWindow) {
	EditorGUIWindow* inspect = gEditorGUIModule.getWindow("Inspect");
	if (inspect != nullptr) {
		inspect->setEnabled(true);
	}
}

DEFINE_ELEMENT_CALLBACK(NameFieldUpdate) {
	EditorElementText* text = dynamic_cast<EditorElementText*>(self);
	if (!selectedNode) {
		text->setContent("Name:");
	}
	else {
		std::string content = "Name:" + selectedNode->name;
		text->setContent(content.c_str());
	}
}

DEFINE_ELEMENT_CALLBACK(PositionFieldUpdate) {
	if (selectedNode == nullptr) return;
	//we only change object's position in relative coordinate
	Vector3 position = selectedNode->transform.GetPosition();
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->setValue(position.raw);
}

DEFINE_ELEMENT_CALLBACK(PositionFieldReadback) {
	if (selectedNode == nullptr) return;
	Vector3 position;
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->getValue(position.raw);
	selectedNode->transform.SetPosition(position);
}

DEFINE_ELEMENT_CALLBACK(RotationFieldUpdate) {
	if (selectedNode == nullptr) return;
	//we only change object's position in relative coordinate
	Vector3 rotation = selectedNode->transform.GetRotation();
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->setValue(rotation.raw);
}

DEFINE_ELEMENT_CALLBACK(RotationFieldReadback) {
	if (selectedNode == nullptr) return;
	Vector3 rotation;
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->getValue(rotation.raw);
	selectedNode->transform.SetRotation(rotation);
}

DEFINE_ELEMENT_CALLBACK(ScaleFieldUpdate) {
	if (selectedNode == nullptr) return;
	//we only change object's position in relative coordinate
	Vector3 scale = selectedNode->transform.GetScale();
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->setValue(scale.raw);
}

DEFINE_ELEMENT_CALLBACK(ScaleFieldReadback) {
	if (selectedNode == nullptr) return;
	Vector3 scale;
	EditorElementInputVector* inputVector = dynamic_cast<EditorElementInputVector*>(self);
	inputVector->getValue(scale.raw);
	selectedNode->transform.SetScale(scale);
}