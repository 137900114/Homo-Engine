#pragma once
#include <vector>
#include "Math.h"
#include <string>

namespace Game {
	class EditorGUIWindow;
	class EditorElementRoot;


	class EditorElement {
	public:

		using EditorElementCallBack = void (*)(EditorElement* self);

		virtual ~EditorElement() {}
		virtual void prase() = 0;
		EditorElement(const char* name,EditorGUIWindow* window = nullptr,EditorElementRoot* root = nullptr) {
			this->window = window;
			this->name = name;
			this->parent = root;

			memset(callbackSlots,0,sizeof(callbackSlots));
		}

		inline void setWindow(EditorGUIWindow* window) { this->window = window; }
		inline EditorGUIWindow* getWindow() { return window; }
		inline const char* getName() { return name.c_str(); }
		inline EditorElementRoot* getParent() { return parent; }
		inline void setParent(EditorElementRoot* root) { parent = root; }
	protected:

		EditorElementCallBack callbackSlots[8];

		EditorGUIWindow* window;
		EditorElementRoot* parent;
		std::string name;
	};

	class EditorElementRoot : public EditorElement{
	public:
		virtual ~EditorElementRoot() {}

		bool addElement(EditorElement* elements);
		bool findElement(EditorElement* elements);
		EditorElement* findElement(const char* name);

		EditorElement* popElement(const char* name);
		bool popElement(EditorElement* element);

		virtual void prase() override;

		EditorElementRoot(const char* name,EditorGUIWindow* window = nullptr): EditorElement(name,window){}
	private:
		std::vector<EditorElement*> elements;
	};

	class EditorGUIWindow {
	public:

		//referenced from imgui.h
		enum EditorGUIWindowFlag{
			EDITOR_GUI_WINDOW_None = 0,
			EDITOR_GUI_WINDOW_NoTitleBar = 1 << 0,   // Disable title-bar
			EDITOR_GUI_WINDOW_NoResize = 1 << 1,   // Disable user resizing with the lower-right grip
			EDITOR_GUI_WINDOW_NoMove = 1 << 2,   // Disable user moving the window
			EDITOR_GUI_WINDOW_NoScrollbar = 1 << 3,   // Disable scrollbars (window can still scroll with mouse or programmatically)
			EDITOR_GUI_WINDOW_NoScrollWithMouse = 1 << 4,   // Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
			EDITOR_GUI_WINDOW_NoCollapse = 1 << 5,   // Disable user collapsing window by double-clicking on it
			EDITOR_GUI_WINDOW_AlwaysAutoResize = 1 << 6,   // Resize every window to its content every frame
			EDITOR_GUI_WINDOW_NoBackground = 1 << 7,   // Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
			EDITOR_GUI_WINDOW_NoSavedSettings = 1 << 8,   // Never load/save settings in .ini file
			EDITOR_GUI_WINDOW_NoMouseInputs = 1 << 9,   // Disable catching mouse, hovering test with pass through.
			EDITOR_GUI_WINDOW_MenuBar = 1 << 10,  // Has a menu-bar
			EDITOR_GUI_WINDOW_HorizontalScrollbar = 1 << 11,  // Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
			EDITOR_GUI_WINDOW_NoFocusOnAppearing = 1 << 12,  // Disable taking focus when transitioning from hidden to visible state
			EDITOR_GUI_WINDOW_NoBringToFrontOnFocus = 1 << 13,  // Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
			EDITOR_GUI_WINDOW_AlwaysVerticalScrollbar = 1 << 14,  // Always show vertical scrollbar (even if ContentSize.y < Size.y)
			EDITOR_GUI_WINDOW_AlwaysHorizontalScrollbar = 1 << 15,  // Always show horizontal scrollbar (even if ContentSize.x < Size.x)
			EDITOR_GUI_WINDOW_AlwaysUseWindowPadding = 1 << 16,  // Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient)
			EDITOR_GUI_WINDOW_NoNavInputs = 1 << 18,  // No gamepad/keyboard navigation within the window
			EDITOR_GUI_WINDOW_NoNavFocus = 1 << 19,  // No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
			EDITOR_GUI_WINDOW_UnsavedDocument = 1 << 20,  // Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. When used in a tab/docking context, tab is selected on closure and closure is deferred by one frame to allow code to cancel the closure (with a confirmation popup, etc.) without flicker.
			EDITOR_GUI_WINDOW_NoNav = EDITOR_GUI_WINDOW_NoNavInputs | EDITOR_GUI_WINDOW_NoNavFocus,
			EDITOR_GUI_WINDOW_NoDecoration = EDITOR_GUI_WINDOW_NoTitleBar | EDITOR_GUI_WINDOW_NoResize | EDITOR_GUI_WINDOW_NoScrollbar | EDITOR_GUI_WINDOW_NoCollapse,
			EDITOR_GUI_WINDOW_NoInputs = EDITOR_GUI_WINDOW_NoMouseInputs | EDITOR_GUI_WINDOW_NoNavInputs | EDITOR_GUI_WINDOW_NoNavFocus,

			EDITOR_GUI_WINDOW_CanBeClosed = 1 << 31
		};

		EditorGUIWindow(const char* name,uint32_t flag = EDITOR_GUI_WINDOW_None,bool activated = true) : root(name,this) {
			this->name = name;
			this->activated = activated;
			this->size = Vector2(-1,-1);
			this->position = Vector2(-1,-1);
			this->flags = flag;
			sizeAdjusted = false;
			positionAdjusted = false;
		}

		void prase();

		inline bool addElement(EditorElement* elements){
			return root.addElement(elements);
		}
		inline bool findElement(EditorElement* elements) {
			return root.findElement(elements);
		}
		inline EditorElement* findElement(const char* name) {
			return root.findElement(name);
		}

		EditorElement* popElement(const char* name) {
			return root.popElement(name);
		}
		bool popElement(EditorElement* element) {
			return root.popElement(element);
		}

		EditorElementRoot* getRootNode() { return &root; }
		
		inline const char* getName() { return name.c_str(); }
		inline void setEnabled(bool enabled) { this->activated = enabled; }

		//we allow the window's position coordinate bigger than the size of the window(which makes the window invisible)
		void setPosition(Vector2 Position);
		//window will adjust it's size according to it's position,set new position before set the new size is recomended
		void setSize(Vector2 Size);

		inline void setFlag(uint32_t flag) {
			this->flags = flag;
		}
	private:
		Vector2 updatePosition();
		Vector2 updateSize(Vector2 updatedPosition);

		EditorElementRoot root;
		std::string name;
		bool activated;

		Vector2 size, position;
		bool sizeAdjusted,positionAdjusted;
		uint32_t flags;
	};
	
	class EditorElementText : public EditorElement {
	public:

		enum TextCallBack {
			UPDATE = 0
		};

		virtual void prase() override;
		EditorElementText (const char* name,const char* content,Color color = ConstColor::White, EditorGUIWindow* window = nullptr):
			EditorElement(name,window){
			this->content = content;
			this->color = color;
		}

		const char* getContent() { return content.c_str(); }
		void setContent(const char* content) { this->content = content; }

		void setCallback(TextCallBack slot, EditorElementCallBack callback) {
			callbackSlots[slot] = callback;
		}
	private:
		std::string content;
		Color color;
	};
	
	class EditorElementHeader : public EditorElementRoot{
	public:
		EditorElementHeader(const char* name,EditorGUIWindow* window = nullptr):
			EditorElementRoot(name,window){}

		virtual void prase() override;
	};


	//-------------Menu Element in the gui-------------//
	class EditorElementMenu;
	class EditorElementMenuBar;

	class EditorElementMenuItem {
		friend class EditorElementMenu;
		friend class EditorElementMenuBar;
	public:
		using MenuItemCallBack = void (*)(EditorElementMenuItem* item,
										  EditorElementMenuBar* menuBar,
										  EditorGUIWindow* window);

		EditorElementMenuItem(const std::string& path, MenuItemCallBack callback, EditorElementMenu* menu, const char* shortcut = nullptr, bool enabled = true);
		void prase();

		inline std::string getPath() { return path; }
	private:
		MenuItemCallBack callback;
		std::string name;
		std::string path;
		std::string shortcut;
		EditorElementMenu* menu;
		bool enabled;
		uint32_t menu_order;
	};

	class EditorElementMenu {
		friend class EditorElementMenuBar;
	public:
		EditorElementMenu(const std::string& path, EditorElementMenuBar* bar, EditorElementMenu* parent = nullptr, bool enabled = true);

		EditorElementMenu* getChildMenu(const std::string& name) {
			for (auto item : childMenu) {
				if (item->name == name) {
					return item;
				}
			}
			return nullptr;
		}
		EditorElementMenuItem* getChildItem(const std::string& name) {
			for (auto item : childItems) {
				if (item->name == name) {
					return item;
				}
			}
			return nullptr;
		}

		inline std::string getChildPath(const std::string& name) {
			return path + "/" + name;
		}

		inline void setEnabled(bool enabled) { this->enabled = enabled; }
		inline bool getEnabled() { return enabled; }

		inline EditorElementMenuBar* getBar() { return bar; }

		void prase();

		bool addChildMenu(EditorElementMenu* menu);
		bool addChildItem(EditorElementMenuItem* item);

		inline std::string getPath() { return path; }

	private:

		std::vector<EditorElementMenuItem*> childItems;
		std::vector<EditorElementMenu*> childMenu;
		bool enabled;
		std::string name;
		std::string path;

		EditorElementMenu* parent;
		EditorElementMenuBar* bar;
		uint32_t menu_order;
		uint32_t menu_size;
	};

	class EditorElementMenuBar : public EditorElement{
	public:
		virtual void prase() override;
		EditorElementMenuBar(const char* name, EditorGUIWindow* window = nullptr, bool mainMenuBar = false);

		EditorElementMenu* getMenu(const std::string& path);
		EditorElementMenuItem* getMenuItem(const std::string& path);

		//insert a menu into the element menu bar accroding to it's path
		bool insertMenu(EditorElementMenu* menu);
		//insert a  menu item into the menu bar accroding to it's path
		bool insertItem(EditorElementMenuItem* item);

	private:

		EditorElementMenu* getMenuByPath(const std::string* path,uint32_t num);

		std::vector<EditorElementMenu*> menu;

		bool (*MenuBarStart)();
		void (*MenuBarEnd)();
	};
	//----------------------------------------------------------------------//


	class EditorElementBlock : public EditorElement {
	public:
		enum CallBack {
			UPDATE = 0
		};

		EditorElementBlock(const char* name, EditorGUIWindow* window = nullptr):
			EditorElement(name,window){}

		void prase();
		void setCallBack(CallBack type,EditorElementCallBack callback);

	};


	class EditorElementInputVector : public EditorElement{
	public:
		enum DEMENSION {
			FLOAT = 1, FLOAT2 = 2, FLOAT3 = 3, FLOAT4 = 4
		};
		enum CallBack {
			UPDATE = 0,
			READBACK = 1
		};

		EditorElementInputVector(const char* name, DEMENSION demension, EditorGUIWindow* window = nullptr):EditorElement(name,window) {
			this->demension = demension;
		}
		
		void setValue(float* value) { memcpy(this->value.raw, value, sizeof(float) * demension); }
		void getValue(float* value) { memcpy(value, this->value.raw, sizeof(float) * demension); }

		uint32_t getDemension() { return demension; }
		virtual void prase() override;

		void setCallBackFunction(CallBack type, EditorElementCallBack callback) {
			callbackSlots[type] = callback;
		}
	private:
		uint32_t demension;
		Vector4 value;
	};

}