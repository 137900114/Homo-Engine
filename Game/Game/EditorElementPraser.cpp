#include "EditorElementPraser.h"
#include "Common.h"
#include "Memory.h"
#include "EditorCallBacks.h"


#include "Reflect_Registry.h"

using namespace tinyxml2;
using namespace Game;

namespace Game {
	extern MemoryModule gMemory;
}

bool EditorElementPraseMenuItem(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,
	EditorElementMenu* parent);
bool EditorElementPraseMenu(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,
	EditorElementMenuBar* bar, EditorElementMenu* parent);

_SETUP_REFLECTION(EditorElementPraser);

#define DEFINE_EDITOR_ELEMENT_PRASER(Func) bool EditorElementPrase##Func##(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,EditorElementRoot* root);\
_REFLECT_REGISTER_TOLOWER(Func, EditorElementPraser,EditorElementPrase)\
bool EditorElementPrase##Func##(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,EditorElementRoot* root)


EditorElementPraser Game::getEditorElementPraserByLabel(const char* name) {
	EditorElementPraser praser = nullptr;
	RefRegister<EditorElementPraser>::find(name, praser);
	return praser;
}


bool CastString2Vector2(std::string str,Vector2& target) {
	std::vector<std::string> str_list;
	str_split(str, str_list,',');

	if (str_list.size() != 2) return false;
	try {
		target[0] = stof(str_list[0]);
		target[1] = stof(str_list[1]);
	}
	catch (...) {
		return false;
	}

	return true;
}


bool Game::EditorElementPraseWindow(tinyxml2::XMLElement* window, EditorGUIWindow* targetWindow) {


	XMLElement* childNode = window->FirstChildElement();
	const char* size_str = window->Attribute("size"),
		* position_str = window->Attribute("position");

 	const char* type = window->Attribute("type");
	if (type != nullptr && !strcmp(type,"CanBeClosed")) {
		targetWindow->setFlag(EditorGUIWindow::EDITOR_GUI_WINDOW_CanBeClosed);
	}

	Vector2 position, size;
	if (position_str != nullptr && CastString2Vector2(position_str,position)) {
		targetWindow->setPosition(position);
	}
	if (size_str != nullptr && CastString2Vector2(size_str,size)) {
		targetWindow->setSize(size);
	}

	while (childNode) {
		const char* nodename = childNode->Name();
		EditorElementPraser praser = getEditorElementPraserByLabel(nodename);

		if (praser != nullptr) {
			praser(childNode, targetWindow,targetWindow->getRootNode());
		}
		else {
			Log("unknown editor element name %s\n",nodename);
		}
		childNode = childNode->NextSiblingElement();
	}

	return true;
}

inline bool praseColorFromText(const char* color,Color& target) {
	if (!ConstColor::GetColorByName(color, target)
		&& !ConstColor::PraseColor(color, target)) {
		Log("fail to prase color text %s\n", color);
		return false;
	}
	return true;
}

//prase element <text> in the xml
DEFINE_EDITOR_ELEMENT_PRASER(Text) {
	static uint32_t textID = 0;
	const char* textName = current->Attribute("name"),
		* textColorName = current->Attribute("color"),
		*updateStr = current->Attribute("update");

	std::string nameBuffer;
	if (textName == nullptr) {
		nameBuffer = "nameless_text_" + std::to_string(textID++);
		textName = nameBuffer.c_str();
	}
	if (textColorName == nullptr) {
		textColorName = "White";
	}
	Color textColor;
	if (!praseColorFromText(textColorName, textColor)) return false;

	const char* textContent = current->FirstChild()->Value();
	EditorElementText* text = gMemory.New<EditorElementText>(textName,textContent,textColor);
	if (!root->addElement(text)) {
		Log("fail to add text %s to the window %s on node %s\n",textName,targetWindow->getName(),root->getName());
		gMemory.Delete(text);
		return false;
	}

	EditorElement::EditorElementCallBack update = EditorCallBackManager::getElementCallBack(updateStr);
	if (update != nullptr) {
		text->setCallback(EditorElementText::TextCallBack::UPDATE, update);
	}

	return true;
}

//prase element <menubar> in the xml
DEFINE_EDITOR_ELEMENT_PRASER(MenuBar){
	static uint32_t barID = 0;
	const char* menuName = current->Attribute("name");
	const char* menuType = current->Attribute("type");
	bool isMainMenuBar = false;
	if (menuType != nullptr && !strcmp(menuType, "main")) {
		isMainMenuBar = true;
	}
	
	std::string nameBuffer;
	if (menuName == nullptr) {
		if (isMainMenuBar) menuName = "main_menu_bar";
		else {
			nameBuffer = "menu_bar_" + std::to_string(barID++);
			menuName = nameBuffer.c_str();
		}
	}
	
	EditorElementMenuBar* bar = gMemory.New<EditorElementMenuBar>(menuName, targetWindow, isMainMenuBar);
	if (!root->addElement(bar)) {
		Log("fail to add menu bar %s to the window %s\n",menuName,targetWindow->getName());
		gMemory.Delete(bar);
		return false;
	}

	XMLElement* menus = current->FirstChildElement("menu");
	while (menus) {
		EditorElementPraseMenu(menus, targetWindow, bar, nullptr);
		menus = menus->NextSiblingElement("menu");
	}

	return true;
}

//prase element header in the xml
DEFINE_EDITOR_ELEMENT_PRASER(Header) {
	const char* name = current->Attribute("name");

	std::string nameBuffer;
	if (name == nullptr) {
		static int ID = 0;
		nameBuffer = "unnamed_header_" + std::to_string(ID++);
		name = nameBuffer.c_str();
	}


	EditorElementHeader* header = gMemory.New<EditorElementHeader>(name);
	if (!root->addElement(header)) {
		Log("fail to add header element %s to window %s at node %s",name,targetWindow->getName(),root->getName());
		gMemory.Delete(header);
		return false;
	}

	tinyxml2::XMLElement* childNode = current->FirstChildElement();
	while (childNode) {
		const char* childName = childNode->Name();
		EditorElementPraser praser = getEditorElementPraserByLabel(childName);
		if (praser != nullptr) {
			praser(childNode, targetWindow, header);
		}
		else {
			Log("encountered invaild label %s while prasing header element %s\n", childName, name);
		}

		childNode = childNode->NextSiblingElement();
	}

	return true;
}

bool EditorElementPraseMenu(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,
	EditorElementMenuBar* bar, EditorElementMenu* parent) {
	const char* name = current->Attribute("name");
	if (name == nullptr) {
		Log("menu element must have a name\n");
		return false;
	}

	std::string path = name;
	if (parent != nullptr) path = parent->getChildPath(path);

	EditorElementMenu* menu = gMemory.New<EditorElementMenu>(path, bar, parent);
	if (parent != nullptr) {
		if (!parent->addChildMenu(menu)) {
			Log("fail to add menu element %s to menu element %s\n",path.c_str(),
				parent->getPath().c_str());
			return false;
		}
	}
	else {
		if (!bar->insertMenu(menu)) {
			Log("fail to add menu element %s to bar element %s\n",path.c_str(),
				bar->getName());
			return false;
		}
	}

	XMLElement* element = current->FirstChildElement();
	while (element != nullptr) {
		const char* element_name = element->Name();
		if (!strcmp(element_name, "menu")) {
			EditorElementPraseMenu(element, targetWindow, bar, menu);
		}
		else if (!strcmp(element_name,"menuitem")) {
			EditorElementPraseMenuItem(element, targetWindow, menu);
		}
		element = element->NextSiblingElement();
	}

	return true;
}


bool EditorElementPraseMenuItem(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,
	EditorElementMenu* parent) {

	const char* name = current->Attribute("name");
	const char* callbackName = current->Attribute("callback");
	const char* shortcut = current->Attribute("shortcut");

	if (name == nullptr) {
		Log("Menu item must have a name\n");
		return false;
	}
	if (callbackName == nullptr) {
		callbackName = "Default";
	}

	EditorElementMenuItem::MenuItemCallBack callback = EditorCallBackManager::getMenuItemCallBack(callbackName);
	if (callback == nullptr) {
		callback = EditorCallBackManager::getMenuItemCallBack("Default");
	}

	std::string path = name;
	path = parent->getChildPath(path);

	EditorElementMenuItem* item = gMemory.New<EditorElementMenuItem>(path, callback, parent, shortcut);
	parent->addChildItem(item);

	return true;
}


DEFINE_EDITOR_ELEMENT_PRASER(Block) {
	const char* name = current->Attribute("name");
	const char* update = current->Attribute("update");

	std::string nameStr;
	if (name == nullptr) {
		static int ID = 0;
		nameStr = "unnamed_block_" + std::to_string(ID);
		name = nameStr.c_str();
	}
	EditorElement::EditorElementCallBack updateCallback;
	if (update == nullptr) {
		updateCallback = nullptr;
	}else
		updateCallback = EditorCallBackManager::getElementCallBack(update);

	EditorElementBlock* block = gMemory.New<EditorElementBlock>(name);
	if (!root->addElement(block)) {
		Log("fail to add block element %s to window %s on node %s",name,targetWindow->getName(),
			root->getName());
		return false;
	}
	block->setCallBack(EditorElementBlock::UPDATE, updateCallback);

	return true;
}


bool EditorElementPraseInputVector(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,
	EditorElementRoot* root, EditorElementInputVector::DEMENSION demension) {
	const char* name = current->Attribute("name");
	const char* updateStr = current->Attribute("update");
	const char* readbackStr = current->Attribute("readback");

	std::string nameBuffer;
	if (name == nullptr) {
		static uint32_t ID = 0;
		nameBuffer = "unnamed_vector_" + std::to_string(ID);
		name = nameBuffer.c_str();
	}

	EditorElement::EditorElementCallBack update = EditorCallBackManager::getElementCallBack(updateStr)
		, readback = EditorCallBackManager::getElementCallBack(readbackStr);

	EditorElementInputVector* inputVector = gMemory.New<EditorElementInputVector>(name, demension,nullptr);
	if (!root->addElement(inputVector)) {
		Log("fail to add input vector element %s to window %s on node %s", name, targetWindow->getName(), root->getName());
		gMemory.Delete(inputVector);
		return false;
	}
	inputVector->setCallBackFunction(EditorElementInputVector::CallBack::UPDATE, update);
	inputVector->setCallBackFunction(EditorElementInputVector::CallBack::READBACK, readback);

	return true;
}


DEFINE_EDITOR_ELEMENT_PRASER(Vector3) {
	return EditorElementPraseInputVector(current,targetWindow,
		root,EditorElementInputVector::FLOAT3);
}

DEFINE_EDITOR_ELEMENT_PRASER(Vector2) {
	return EditorElementPraseInputVector(current, targetWindow,
		root, EditorElementInputVector::FLOAT2);
}
DEFINE_EDITOR_ELEMENT_PRASER(Float) {
	return EditorElementPraseInputVector(current, targetWindow,
		root, EditorElementInputVector::FLOAT);
}
DEFINE_EDITOR_ELEMENT_PRASER(Vector4) {
	return EditorElementPraseInputVector(current, targetWindow,
		root, EditorElementInputVector::FLOAT4);
}