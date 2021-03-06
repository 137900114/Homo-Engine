#include "WindowsApplication.h"
#include "Common.h"
#include "InputBuffer.h"

#include "compile_config.h"
#ifdef ENGINE_MODE_EDITOR
#include "EditorGUIModule.h"
#include "GraphicModule.h"

namespace Game {
	extern EditorGUIModule gEditorGUIModule;
	extern GraphicModule* gGraphic;
	extern IApplication* app;
}
#endif

using namespace Game;


LRESULT CALLBACK WinProc(HWND,UINT,WPARAM,LPARAM);

namespace Game {
	extern InputBuffer gInput;
};

bool WindowsApplication::initialize() {
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	hinstance = GetModuleHandle(NULL);

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hinstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass";
	wc.lpfnWndProc = WinProc;

	RegisterClassEx(&wc);

	winHandle = CreateWindowEx(0,
		L"WindowClass", L"Homo",
		WS_OVERLAPPEDWINDOW,
		0, 0, config.width, config.height,
		NULL, NULL, hinstance, NULL);

	if (winHandle == NULL) {
		Log("fail to initialize window\n");
		return false;
	}

	ShowWindow(winHandle, SW_SHOWDEFAULT);
	UpdateWindow(winHandle);

	RECT rect;
	::GetClientRect(winHandle, &rect);

	config.height = rect.bottom - rect.top; 
	config.width = rect.right - rect.left;

	//return  backEnd? backEnd->initialize() : true;
	for (auto iter : moduleList) {
		if (!iter->initialize()) {
			return false;
		}
	}

	return true;
}

bool WindowsApplication::isQuit() {
	return quit;
}

void WindowsApplication::Quit() {
	this->quit = true;
}

void WindowsApplication::tick() {
	
	MSG msg = {};
	if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else {
		for (auto iter : moduleList) {
			iter->tick();
		}
		
		return;
	}
	quit = msg.message == WM_QUIT;
}

void WindowsApplication::finalize() {
	auto iter = moduleList.rbegin();
	while (iter != moduleList.rend()) {
		(*iter)->finalize();
		iter++;
	}
}

LRESULT CALLBACK WinProc(HWND handle,UINT Msg,
	WPARAM wParam,LPARAM lParam) {

#ifdef  ENGINE_MODE_EDITOR
	if (gEditorGUIModule.msgHandler(handle,Msg,wParam,lParam)) {
		return true;
	}
#endif


#define GET_X_FROM_LPARAM(lParam) lParam & 0xffff
#define GET_Y_FROM_LPARAM(lParam) lParam >> 16
	int keycode;
	switch (Msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDOWN:
		gInput.BufferWriteKeyDown(InputBuffer::KeyCode::MOUSE_LEFT);
		break;
	case WM_RBUTTONDOWN:
		gInput.BufferWriteKeyDown(InputBuffer::KeyCode::MOUSE_RIGHT);
		break;
	case WM_MBUTTONDOWN:
		gInput.BufferWriteKeyDown(InputBuffer::KeyCode::MOUSE_MIDDLE);
		break;
	case WM_MBUTTONUP:
		gInput.BufferWriteKeyUp(InputBuffer::KeyCode::MOUSE_MIDDLE);
		break;
	case WM_LBUTTONUP:
		gInput.BufferWriteKeyUp(InputBuffer::KeyCode::MOUSE_LEFT);
		break;
	case WM_RBUTTONUP:
		gInput.BufferWriteKeyUp(InputBuffer::KeyCode::MOUSE_RIGHT);
		break;
	case WM_MOUSEMOVE:
		gInput.BufferWriteMousePosition(GET_X_FROM_LPARAM(lParam),GET_Y_FROM_LPARAM(lParam));
		break;
	case WM_KEYDOWN:
		keycode = wParam - 0x41;
		if (keycode >= 0 && keycode < 26) {
			gInput.BufferWriteKeyDown((InputBuffer::KeyCode)keycode);
		}
		break;
	case WM_KEYUP:
		keycode = wParam - 0x41;
		if (keycode >= 0 && keycode < 26) {
			gInput.BufferWriteKeyUp((InputBuffer::KeyCode)keycode);
		}
		break;
	case WM_SIZE:
#ifdef ENGINE_MODE_EDITOR
		app->resize(LOWORD(lParam),HIWORD(lParam));

		gGraphic->resize();
		gEditorGUIModule.resize();
#endif
		break;
	}

	return DefWindowProc(handle, Msg, wParam, lParam);
}

void Game::WindowsApplication::setTitle(const char* title) {
	SetWindowTextA(winHandle,title);
}

void Game::WindowsApplication::resize(uint32_t width,uint32_t height) {
	config.width = width;
	config.height = height;
}


//---------------------------------------------------------//
//this part of code is modified from https://blog.csdn.net/coolsunxu/article/details/82915531

std::string Lpcwstr2String(LPCWSTR lps) {
	int len = WideCharToMultiByte(CP_ACP, 0, lps, -1, NULL, 0, NULL, NULL);
	if (len <= 0) {
		return "";
	}
	else {
		char* dest = new char[len];
		WideCharToMultiByte(CP_ACP, 0, lps, -1, dest, len, NULL, NULL);
		dest[len - 1] = 0;
		std::string str(dest);
		delete[] dest;
		return str;
	}
}

LPCWSTR filterTranslater(const char* filter) {
	return L"All Files\0*.*\0\0";
}

std::string Game::WindowsApplication::getPath(const char* filter) {
	OPENFILENAME ofn;
	char szFile[300];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = (LPWSTR)szFile;
	ofn.lpstrFile[0] = '\0';
	LPTSTR        lpstrCustomFilter;
	DWORD         nMaxCustFilter;
	ofn.nFilterIndex = 1;
	LPTSTR        lpstrFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filterTranslater(filter);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	std::string path_image = "";
	if (GetOpenFileName(&ofn)) {
		path_image = Lpcwstr2String(ofn.lpstrFile);
		return path_image;
	}
	else {
		return "";
	}
}

//----------------------------------------------------------//