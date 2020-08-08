#include "WindowsApplication.h"
#include "D3DRenderModule.h"
#include "Memory.h"
#include "FileLoader.h"
#include "Timer.h"
#include "InputBuffer.h"
#include "GeometryGenerator.h"

//build the modules work together

namespace Game {
	class Test : public IRuntimeModule{
	public:
		void tick() override;
		bool initialize() override;
		void finalize() override {  }
	private:
		Texture t;
		Mesh mesh;
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

	SceneLoader gSceneLoader;
	Timer gTimer;

#ifdef _WIN64
	IRuntimeModule* moduleList[] = {&mem,gFileLoader,gGraphic,&t,&gTimer,&gInput};
	WindowsApplication wapp(1200, 750, moduleList, _countof(moduleList));

	IApplication* app = &wapp;
#endif


	bool Test::initialize() {
		/*image[0] = gImageLoader.loadImage("2.bmp");
		image[1] = gImageLoader.loadImage("1.bmp");
		CubeMapFileArray files;
		files.front = "DefaultSkybox\\default_skybox_front.bmp";
		files.back = "DefaultSkybox\\default_skybox_back.bmp";
		files.right = "DefaultSkybox\\default_skybox_right.bmp";
		files.left = "DefaultSkybox\\default_skybox_left.bmp";
		files.up = "DefaultSkybox\\default_skybox_up.bmp";
		files.down = "DefaultSkybox\\default_skybox_down.bmp";

		cubeImage = gImageLoader.loadCubeMap(files);
		*/

		//gGraphic->set2DViewPort(Vector2(), 2.);
		
		mesh = std::move(GeometryGenerator::generateCube(false));

		gGraphic->drawSingleMesh(mesh);
		//t = std::move(gImageLoader.loadImage("DefaultSkyBox\\default_skybox_back.bmp"));

		return true;
	}

	void Test::tick() {
		//gGraphic->image2D(t, Vector2(0., 0.), Vector2(.5, .5), 0., 0.3);
		//gGraphic->image2D(image[1], Vector2(0., -.25), Vector2(.5, .25), 0., .3);
		//gGraphic->point2D(Vector2(), 0.2, ConstColor::Black);
	}
}
