#include "WindowsApplication.h"
#include "D3DRenderModule.h"
#include "Memory.h"
#include "FileLoader.h"
#include "Timer.h"
#include "InputBuffer.h"

//build the modules work together

namespace Game {
	MemoryModule mem;
	MemoryModule* gMemory = &mem;

#ifdef _WIN64
	D3DRenderModule d3d;
	GraphicModule* gGraphic = &d3d;
#endif

	FileLoader fileloader;
	FileLoader* gFileLoader = &fileloader;

	InputBuffer gInput;

	Timer gTimer;

#ifdef _WIN64
	IRuntimeModule* moduleList[] = {&mem,gFileLoader,gGraphic,&gTimer,&gInput};
	WindowsApplication wapp(1200, 750, moduleList, _countof(moduleList));

	IApplication* app = &wapp;
#endif
}
