#include "WindowsApplication.h"
#include "D3DRenderModule.h"
#include "Memory.h"
#include "FileLoader.h"
#include "Timer.h"
#include "InputBuffer.h"

//build the modules work together

class Test : public Game::IRuntimeModule {
public:
	bool initialize();
	void tick() {}
	void finalize() {}
private:

};


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

	SceneLoader gSceneLoader;
	Test t;

	Timer gTimer;

#ifdef _WIN64
	IRuntimeModule* moduleList[] = { &mem,gFileLoader,gGraphic,&t,&gTimer,&gInput };
	WindowsApplication wapp(1200, 750, moduleList, _countof(moduleList));

	IApplication* app = &wapp;
#endif
}


bool Test::initialize() {
	Game::Scene* scene = Game::gSceneLoader.loadScene("2.obj");
	Game::gGraphic->drawSingleMesh(scene->meshs["Suzanne"]);
	return true;
}