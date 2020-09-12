#include "WindowsApplication.h"
#include "D3DRenderModule.h"
#include "Memory.h"
#include "FileLoader.h"
#include "Timer.h"
#include "InputBuffer.h"
#include "GeometryGenerator.h"
#include "MeshRenderer.h"

//build the modules work together

namespace Game {
	class Test : public IRuntimeModule{
	public:
		void tick() override;
		bool initialize() override;
		void finalize() override {  }
	private:
		//Mesh mesh;
		//Scene* scene;
		//Texture tex[2];
		Texture sky;
		Mesh box;
	};


	MemoryModule mem;
	MemoryModule* gMemory = &mem;

#ifdef _WIN64
	D3DRenderModule d3d;
	GraphicModule* gGraphic = &d3d;
#endif

	FileLoader fileloader;
	FileLoader* gFileLoader = &fileloader;

	InputBuffer gInput;
	TextureLoader gImageLoader;
	Test t;

	//SceneLoader gSceneLoader;
	SceneManager gSceneManager;
	Timer gTimer;

	extern SceneLoader gSceneLoader;

	MeshDrawCallMaker gMeshDrawCallMaker;
#ifdef _WIN64
	IRuntimeModule* moduleList[] = {&mem,gFileLoader,&gSceneManager,&gMeshDrawCallMaker,gGraphic,&t,&gTimer,&gInput};
	WindowsApplication wapp(1920, 1080, moduleList, _countof(moduleList));

	IApplication* app = &wapp;
#endif

	bool Test::initialize() {

		gSceneManager.loadScene("test_scene1.dae",true);
		
		SceneCamera* camera = gSceneManager.GetMainCamera();
		camera->transform.Rotation.x = 6.505;
		camera->transform.Position = Vector3(-12, 5.5, -12);

		SceneLight* light = gSceneManager.GetMainLight();
		light->transform.Position = Vector3(0.,0.,-1.);


		CubeMapFileArray cube;
		cube.back = "DefaultSkyBox\\default_skybox_back.bmp";
		cube.front = "DefaultSkyBox\\default_skybox_front.bmp";
		cube.down = "DefaultSkyBox\\default_skybox_down.bmp";
		cube.left = "DefaultSkyBox\\default_skybox_left.bmp";
		cube.right = "DefaultSkyBox\\default_skybox_right.bmp";
		cube.up = "DefaultSkyBox\\default_skybox_up.bmp";

		sky = std::move(gImageLoader.loadCubeMap(cube));
		gGraphic->skybox(sky, camera);
		gGraphic->set2DViewPort(Vector2(0., 0.),3.);

		return true;
	}

	void Test::tick() {
		if (gInput.KeyHold(InputBuffer::A))
			gSceneManager.GetMainCamera()->transform.Rotation.y -= gTimer.DeltaTime();
		else if (gInput.KeyHold(InputBuffer::D))
			gSceneManager.GetMainCamera()->transform.Rotation.y += gTimer.DeltaTime();
		else if (gInput.KeyHold(InputBuffer::W))
			gSceneManager.GetMainCamera()->transform.Rotation.x -= gTimer.DeltaTime();
		else if (gInput.KeyHold(InputBuffer::S))
			gSceneManager.GetMainCamera()->transform.Rotation.x += gTimer.DeltaTime();
		else if (gInput.KeyHold(InputBuffer::Q))
			gSceneManager.GetMainCamera()->transform.Position.x += gTimer.DeltaTime();
		else if (gInput.KeyHold(InputBuffer::E))
			gSceneManager.GetMainCamera()->transform.Position.x -= gTimer.DeltaTime();

	}
}
