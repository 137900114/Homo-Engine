#include "EditorElements.h"
#include "imgui/imgui.h"
#include "Common.h"
#include "IApplication.h"

using namespace Game;

namespace Game {
	extern IApplication* app;
}

void EditorGUIWindow::prase() {
	if (activated) {
		if (positionAdjusted) {
			Vector2 newPos = updatePosition();
			ImGui::SetNextWindowPos(ImVec2(newPos.x, newPos.y));
			if (sizeAdjusted) {
				Vector2 newSize = updateSize(newPos);
				ImGui::SetNextWindowSize(ImVec2(newSize.x, newSize.y));
			}
		}
		else if(sizeAdjusted){
			Vector2 newSize = updateSize(Vector2());
			ImGui::SetNextWindowSize(ImVec2(newSize.x, newSize.y));
		}
		
		if (this->flags & EDITOR_GUI_WINDOW_CanBeClosed) {
			uint32_t flag = flags & ~(EDITOR_GUI_WINDOW_CanBeClosed);
			if (ImGui::Begin(this->name.c_str(), &activated,flag)) {
				root.prase();
				ImGui::End();
			}

			if (!activated) {
				int i = 0;
			}
		}
		else {
			ImGui::Begin(this->name.c_str(),nullptr,flags);
			root.prase();
			ImGui::End();
		}
	}
}

bool EditorElementRoot::addElement(EditorElement* element) {
	if (findElement(element->getName()) != nullptr) {
		return false;
	}

	element->setWindow(this->getWindow());
	element->setParent(this);
	elements.push_back(element);
	return true;
}

EditorElement* EditorElementRoot::findElement(const char* name) {
	for (auto item : elements) {
		if (!strcmp(item->getName(),name)) {
			return item;
		}
	}
	return nullptr;
}

bool EditorElementRoot::findElement(EditorElement* element) {
	for (auto item : elements) {
		if (element == item) return true;
	}
	return false;
}

EditorElement* EditorElementRoot::popElement(const char* name) {
	for (auto iter = elements.begin(); iter != elements.end(); iter++) {
		if (!strcmp((*iter)->getName(),name)) {
			EditorElement* element = *iter;
			elements.erase(iter);
			return element;
		}
	}

	return nullptr;
}

bool EditorElementRoot::popElement(EditorElement* element) {
	for (auto iter = elements.begin(); iter != elements.end(); iter++) {
		if (*iter == element) {
			elements.erase(iter);
			return true;
		}
	}
	return false;
}

void EditorElementRoot::prase() {
	for (auto item : elements) {
		item->prase();
	}
}

void EditorElementText::prase() {
	if (callbackSlots[UPDATE] != nullptr) {
		callbackSlots[UPDATE](this);
	}

	ImColor texcolor(color[0],color[1],color[2],color[3]);
	
	ImGui::TextColored(texcolor,content.c_str());
}

using PathArray = std::vector<std::string>;
inline void getPathArray(const std::string& path,PathArray& pathArray) {
	str_split(path,pathArray,'/');
}


EditorElementMenu* EditorElementMenuBar::getMenuByPath(const std::string* path,uint32_t num) {
	EditorElementMenu* current = nullptr;
	for (auto& item : menu) {
		if (item->name == path[0]) {
			current = item;
		}
	}
	if (current == nullptr) 
		return nullptr;

	for (int i = 1; i < num; i++) {
		const std::string& current_node = path[i];

		current = current->getChildMenu(current_node);
		if (current == nullptr) {
			return nullptr;
		}
	}

	return current;
}

EditorElementMenu* EditorElementMenuBar::getMenu(const std::string& path) {
	PathArray pathArray;
	getPathArray(path, pathArray);
	return getMenuByPath(pathArray.data(), pathArray.size());
}

EditorElementMenuItem* EditorElementMenuBar::getMenuItem(const std::string& path) {
	PathArray pathArray;
	getPathArray(path,pathArray);

	EditorElementMenu* parentItem = getMenuByPath(pathArray.data(), pathArray.size() - 1);
	if (parentItem == nullptr) {
		return nullptr;
	}

	return parentItem->getChildItem(pathArray[pathArray.size() - 1]);
}

bool EditorElementMenuBar::insertMenu(EditorElementMenu* menu) {
	PathArray pathArray;
	getPathArray(menu->path,pathArray);

	if (pathArray.size() == 1) {
		if (getMenuByPath(pathArray.data(),1) != nullptr) {
			return false;
		}
		this->menu.push_back(menu);
		return true;
	}
	
	EditorElementMenu* parentItem = getMenuByPath(pathArray.data(), pathArray.size() - 1);
	if (parentItem == nullptr) return false;
	return parentItem->addChildMenu(menu);
}

bool EditorElementMenuBar::insertItem(EditorElementMenuItem* item) {
	EditorElementMenu* parentItem = item->menu;
	return parentItem->addChildItem(item);
}

void EditorElementMenuBar::prase() {

	if (MenuBarStart()) {
		for (auto item : menu)
			item->prase();

		MenuBarEnd();
	}
}

EditorElementMenuBar::EditorElementMenuBar(const char* name, EditorGUIWindow* window, bool mainMenuBar) :
	EditorElement(name, window) {
	if (mainMenuBar) {
		MenuBarStart = ImGui::BeginMainMenuBar;
		MenuBarEnd = ImGui::EndMainMenuBar;
	}
	else {
		MenuBarStart = ImGui::BeginMenuBar;
		MenuBarEnd = ImGui::EndMenuBar;
	}
}




EditorElementMenu::EditorElementMenu(const std::string& path, EditorElementMenuBar* bar, EditorElementMenu* parent, bool enabled) {
	this->bar = bar;
	this->parent = parent;
	this->enabled = enabled;

	PathArray pathArray;
	getPathArray(path, pathArray);

	this->path = path;
	this->name = pathArray[pathArray.size() - 1];
	this->menu_size = 0;
}

void EditorElementMenu::prase() {
	if (enabled) {
		if (ImGui::BeginMenu(name.c_str())) {
			int menuIndex = 0,itemIndex = 0;
			while (menuIndex < childMenu.size()
				|| itemIndex < childItems.size()) {
				if (menuIndex >= childMenu.size()) {
					childItems[itemIndex++]->prase();
				}
				else if (itemIndex >= childItems.size()) {
					childMenu[menuIndex++]->prase();
				}else if (childMenu[menuIndex]->menu_order > childItems[itemIndex]->menu_order) {
					childItems[itemIndex++]->prase();
				}else {
					childMenu[menuIndex++]->prase();
				}
			}
			ImGui::EndMenu();
		}
	}

}

bool EditorElementMenu::addChildMenu(EditorElementMenu* menu) {
	PathArray pathArray,myPathArray;
	getPathArray(menu->path, pathArray);
	getPathArray(this->path, myPathArray);
	if (pathArray.size() != 1 + myPathArray.size()) return false;

	for (int i = 0; i != myPathArray.size(); i++) {
		if (pathArray[i] != myPathArray[i])
			return false;
	}

	if ( getChildMenu( pathArray[pathArray.size() - 1]) != nullptr) {
		return false;
	}

	this->childMenu.push_back(menu);
	menu->menu_order = this->menu_size++;
	return true;
}

bool EditorElementMenu::addChildItem(EditorElementMenuItem* item) {
	PathArray pathArray, myPathArray;
	getPathArray(item->path, pathArray);
	getPathArray(this->path, myPathArray);

	if (pathArray.size() != 1 + myPathArray.size()) return false;

	for (int i = 0; i != myPathArray.size(); i++) {
		if (pathArray[i] != myPathArray[i]) return false;
	}

	if (getChildItem(item->name) != nullptr) {
		return false;
	}

	this->childItems.push_back(item);
	item->menu_order = this->menu_size++;
	return true;
}

EditorElementMenuItem::EditorElementMenuItem(const std::string& path, MenuItemCallBack callback, EditorElementMenu* menu, const char* shortcut , bool enabled) {
	this->enabled = true;
	this->shortcut = shortcut != nullptr ? shortcut : "";
	this->menu = menu;

	this->callback = callback;

	PathArray pathArray;
	getPathArray(path, pathArray);
	this->path = path;
	this->name = pathArray[pathArray.size() - 1];
}

void EditorElementMenuItem::prase() {

	if (this->enabled) {
		bool selected = shortcut.empty() ? ImGui::MenuItem(name.c_str()) : ImGui::MenuItem(name.c_str(), shortcut.c_str());
		if (selected && callback != nullptr) {

			EditorElementMenuBar* menuBar = menu->getBar();
			EditorGUIWindow* window = menuBar->getWindow();

			callback(this, menuBar, window);
		}
	}
}

void EditorElementHeader::prase() {
	if (!ImGui::CollapsingHeader(getName())) {
		EditorElementRoot::prase();
	}
}

void EditorElementBlock::prase() {
	if(callbackSlots[CallBack::UPDATE] != nullptr)
		callbackSlots[CallBack::UPDATE](this);
}

void EditorElementBlock::setCallBack(CallBack type,EditorElementCallBack callback) {
	callbackSlots[type] = callback;
}

void EditorElementInputVector::prase() {

	constexpr float step = 1e-4;
	constexpr float stepfast = 1e-2;
	constexpr int precision = 3;

	if (callbackSlots[UPDATE]) {
		callbackSlots[UPDATE](this);
	}
	switch (demension) {
	case FLOAT:
		ImGui::InputFloat(name.c_str(), value.raw, step, stepfast, precision);
		break;
	case FLOAT2:
		ImGui::InputFloat2(name.c_str(), value.raw, precision);
		break;
	case FLOAT3:
		ImGui::InputFloat3(name.c_str(), value.raw, precision);
		break;
	case FLOAT4:
		ImGui::InputFloat4(name.c_str(), value.raw, precision);
		break;
	}

	if (callbackSlots[READBACK]) {
		callbackSlots[READBACK](this);
	}
}


void EditorGUIWindow::setPosition(Vector2 Position) {
	positionAdjusted = true;

	this->position = Position;
}

Vector2 EditorGUIWindow::updatePosition() {
	Config appSize = app->getSysConfig();
	Vector2 newposition;
	if (position.x < 0) {
		newposition.x = fmax(appSize.width + position.x, 0.);
	}
	else {
		newposition.x = position.x;
	}

	if (position.y < 0) {
		newposition.y == fmax(appSize.height + position.y, 0.);
	}
	else {
		newposition.y = position.y;
	}
	return newposition;
}


void EditorGUIWindow::setSize(Vector2 size) {
	sizeAdjusted = true;
	this->size = size;
}

Vector2 EditorGUIWindow::updateSize(Vector2 position) {
	Config appSize = app->getSysConfig();
	Vector2 updatedSize;
	if (position.x > appSize.width || position.y > appSize.height) return Vector2(0.,0.);

	if (size.x > 0.) {
		updatedSize.x = fmax(0., fmin(appSize.width - position.x, size.x));
	}
	else {
		updatedSize.x = fmax(0., appSize.width - position.x + size.x);
	}

	if (size.y > 0.) {
		updatedSize.y = fmax(0., fmin(appSize.height - position.y, size.y));
	}
	else {
		updatedSize.y = fmax(0., appSize.height - position.y + size.y);
	}
	return updatedSize;
}

