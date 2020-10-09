#include "WindowsApplication.h"
#include "D3DRenderModule.h"
#include "Memory.h"
#include "FileLoader.h"
#include "Timer.h"
#include "InputBuffer.h"
#include "GeometryGenerator.h"
#include "MeshRenderer.h"
#include "PhysicsSimulater.h"

//build the modules work together

#include "PhyParticalComponent.h"
#include "compile_config.h"
#include "EditorGUIModule.h"

#include "FPSCamera.h"

namespace Game {
	class Test : public IRuntimeModule {
	public:
		void tick() override;
		bool initialize() override;
		void finalize() override {  }
	private:

		Texture sky;
	};

	MemoryModule gMemory;

#ifdef _WIN64
	D3DRenderModule d3d;
	GraphicModule* gGraphic = &d3d;
#endif

#ifdef ENGINE_MODE_EDITOR
	EditorGUIModule gEditorGUIModule;
#endif

	FileLoader gFileLoader;

	InputBuffer gInput;
	TextureLoader gImageLoader;
	Test t;

	//SceneLoader gSceneLoader;
	SceneManager gSceneManager;
	Timer gTimer;

	PhysicsSimulater gPhysicsSimulater;

	MeshDrawCallMaker gMeshDrawCallMaker;
#ifdef _WIN64
	IRuntimeModule* moduleList[] = { &gMemory,&gFileLoader,&gPhysicsSimulater,
		&gSceneManager,&gMeshDrawCallMaker,gGraphic,
#ifdef ENGINE_MODE_EDITOR
		&gEditorGUIModule,
#endif
		&t,&gTimer,&gInput };

	WindowsApplication wapp(1920, 1080, moduleList, _countof(moduleList));

	IApplication* app = &wapp;
#endif

	bool Test::initialize() {
		return true;
	}

	void Test::tick() {	
		
	}

}
 