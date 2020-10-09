#pragma once
#include "EditorElements.h"

namespace Game {

	class EditorCallBackManager {
	public:
		static EditorElementMenuItem::MenuItemCallBack getMenuItemCallBack(const char* name);
		static EditorElement::EditorElementCallBack getElementCallBack(const char* name);
	};

}