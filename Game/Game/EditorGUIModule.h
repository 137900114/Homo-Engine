#pragma once
#include "IRuntimeModule.hpp"
#include "EditorElements.h"

#ifdef _WIN64
#include "D3DRenderModule.h"
#endif

namespace Game {

	class EditorGUIModule : public IRuntimeModule{
	public:
		bool initialize() override;
		void finalize() override;
		
		void tick() override;
#ifdef _WIN64
		bool msgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void draw(ID3D12GraphicsCommandList* pCommandList);
#endif
		void resize();
		EditorGUIWindow* getWindow(const char* name);
	private:

		bool createTheGUI();
		std::vector<EditorGUIWindow> windows;
		bool isInitalized = false;
	};
}