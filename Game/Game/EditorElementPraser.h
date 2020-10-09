#pragma once

#include "tinyxml2.h"
#include "EditorElements.h"

namespace Game {

	using EditorElementPraser = bool (*)(tinyxml2::XMLElement* current, EditorGUIWindow* targetWindow,EditorElementRoot* root);
	EditorElementPraser getEditorElementPraserByLabel(const char* name);

	bool EditorElementPraseWindow(tinyxml2::XMLElement* window, EditorGUIWindow* targetWindow);
}