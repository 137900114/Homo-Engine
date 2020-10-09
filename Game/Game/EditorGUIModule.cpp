#include "EditorGUIModule.h"
#include "imgui/imgui.h"

#include "GraphicModule.h"
#include "IApplication.h"

#include "Common.h"

#ifdef _WIN64
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include "WindowsApplication.h"
#endif

#include "EditorCallBacks.h"
#include "Memory.h"
#include "FileLoader.h"
#include "tinyxml2.h"

#include "EditorElementPraser.h"
#include "FPSCamera.h"
#include "Scene.h"

#include "InputBuffer.h"
#include "Timer.h"

namespace Game {
	extern IApplication* app;
	extern GraphicModule* gGraphic;
	extern MemoryModule gMemory;
	extern FileLoader gFileLoader;
	extern TextureLoader gImageLoader;

	FPSCamera fpsCamera;
	extern SceneManager gSceneManager;
	extern InputBuffer gInput;

	extern Timer gTimer;
}


#ifdef _WIN64
ComPtr<ID3D12DescriptorHeap> gui_heap;
#endif

using namespace Game;

bool EditorGUIModule::initialize() {
#ifdef _WIN64

	D3DRenderModule* d3dGraphic = dynamic_cast<D3DRenderModule*>(gGraphic);
	WindowsApplication* winApp = dynamic_cast<WindowsApplication*>(app);

	HWND winHandle = winApp->winHandle;


	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ID3D12Device* device = d3dGraphic->mDevice.Get();

	D3D12_DESCRIPTOR_HEAP_DESC gui_heap_desc;
	gui_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	gui_heap_desc.NodeMask = 0;
	gui_heap_desc.NumDescriptors = 1;

	gui_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	if (FAILED(device->CreateDescriptorHeap(&gui_heap_desc,IID_PPV_ARGS(&gui_heap)))) {
		Log("fail to create descriptor heap for editer gui,program quit\n");
		return false;
	}
	
	if (!ImGui_ImplWin32_Init(winHandle)) {
		Log("fail to initialize Imgui win32 for editer gui,program quit\n");
		return false;
	}

	if (!ImGui_ImplDX12_Init(device, d3dGraphic->mBackBufferNum,
		d3dGraphic->mBackBufferFormat, gui_heap.Get(),
		gui_heap->GetCPUDescriptorHandleForHeapStart(),
		gui_heap->GetGPUDescriptorHandleForHeapStart())) {

		Log("fail to initialize d3d12 for Imgui,program quit\n");
		return false;
	}

	if (!createTheGUI()) { return false; }

	isInitalized = true;

	return true;
#else
	Log("fail to initialize the editer gui module,the plantform is not supported\n");
	return false;
#endif
}


bool EditorGUIModule::createTheGUI() {

	Buffer editorFileData;
	if (!gFileLoader.FileReadAndClose("Editor\\Editor.xml", editorFileData)) {
		Log("fail to read editor gui data from file Editor\\Editor.xml\n");
		return false;
	}

	tinyxml2::XMLDocument document;
	if (document.Parse(reinterpret_cast<const char*>(editorFileData.data), editorFileData.size)) {
		Log("fail to prase xml file Editor\\Editor.xml\n");
		return false;
	}

	tinyxml2::XMLElement* editor_element = document.FirstChildElement("editor");
	if (!editor_element) {
		Log("editor gui file should start with element <editor>!\n");
		return false;
	}

	tinyxml2::XMLElement* window = editor_element->FirstChildElement("window");
	while (window) {
		const char* name = window->Attribute("name");
		if (name == nullptr) {
			Log("a editor GUI window must have a name\n");
			window = window->NextSiblingElement("window");
			continue;
		}

		bool isNameRepeated = false;
		for (auto win : windows) {
			if (!strcmp(win.getName(),name)) {
				Log("name %s is used by another window,every window should have their own unqiue name!\n",name);
				isNameRepeated = true;
				break;
			}
		}
		if (isNameRepeated) continue;

		const char* enabledStr = window->Attribute("enabled");
		bool enabled = false;
		if (enabledStr != nullptr && !strcmp(enabledStr, "true")) {
			enabled = true;
		}

		this->windows.push_back(EditorGUIWindow(name,EditorGUIWindow::EDITOR_GUI_WINDOW_None,enabled));


		EditorElementPraseWindow(window, &windows[windows.size() - 1]);
		window = window->NextSiblingElement("window");
	}

	


	return true;
}

void EditorGUIModule::finalize() {

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	gui_heap = nullptr;
}

#ifdef _WIN64
void EditorGUIModule::draw(ID3D12GraphicsCommandList* pCommandList) {

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (auto& window : windows) {
		window.prase();
	}

	ImGui::Render();

	ID3D12DescriptorHeap* heaps[] = {gui_heap.Get()};
	pCommandList->SetDescriptorHeaps(1,heaps);
	ImDrawData* data =ImGui::GetDrawData();
	ImGui_ImplDX12_RenderDrawData( data, pCommandList);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool EditorGUIModule::msgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
#endif

void EditorGUIModule::resize() {
#ifdef _WIN64
	if (!isInitalized) return;

	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();
#endif
	SceneCamera* sceneCamera = gSceneManager.GetMainCamera();
	if (sceneCamera == nullptr) return;
	Config sysconfig = app->getSysConfig();
	sceneCamera->SetAspect((float)sysconfig.width / (float)sysconfig.height);

	

}

EditorGUIWindow* EditorGUIModule::getWindow(const char* name) {
	for (auto& window : windows) {
		if (!strcmp(window.getName(),name)) {
			return &window;
		}
	}
	return nullptr;
}




void EditorGUIModule::tick() {
	SceneCamera* sceneCamera = gSceneManager.GetMainCamera();
	if (gSceneManager.GetMainCamera() == nullptr) {
		return;
	}
	if (sceneCamera != fpsCamera.getSceneCamera()) {
		fpsCamera.attach(sceneCamera);
	}

	static Vector2 lastPos;
	float deltatime = gTimer.DeltaTime();

	if (gInput.KeyDown(InputBuffer::MOUSE_MIDDLE)) {
		lastPos = gInput.MousePosition();
	}
	else if (gInput.KeyHold(InputBuffer::MOUSE_MIDDLE)) {
		Vector2 currentPos = gInput.MousePosition();
		Vector2 delta = currentPos - lastPos;
		lastPos = currentPos;
		constexpr float speed = 0.35;

		fpsCamera.rotateX(delta.x * speed * deltatime);
		fpsCamera.rotateY(delta.y * speed * deltatime);
	}

	constexpr float walkSpeed = 5.f;
	if (gInput.KeyHold(InputBuffer::W)) {
		fpsCamera.walk(walkSpeed * deltatime);
	}
	else if (gInput.KeyHold(InputBuffer::S)) {
		fpsCamera.walk(-walkSpeed * deltatime);
	}
	if (gInput.KeyHold(InputBuffer::A)) {
		fpsCamera.strafe(-walkSpeed * deltatime);
	}
	else if (gInput.KeyHold(InputBuffer::D)) {
		fpsCamera.strafe(walkSpeed * deltatime);
	}
	

	fpsCamera.tick();
}