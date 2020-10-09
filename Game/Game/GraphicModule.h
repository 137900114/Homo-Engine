#pragma once
#include "IRuntimeModule.hpp"
#include "Vector.h"
#include "Texture.h"
#include "Common.h"
#include "Scene.h"
#include "DrawCall.h"

namespace Game {

	class GraphicModule : public IRuntimeModule {
	public:
		

		virtual bool initialize() = 0;
		virtual void tick() = 0;
		virtual void finalize() = 0;

		//The 2D rendering part

		//Draw a point at position pos on the screen.The depth determines the covering order on the screen
		//Smaller the depth value the geometry will be more likely to cover other geometries
		//For exmaple a point which has depth value 0.5 will cover the point which has depth value 0.7
		//at the same place
		virtual void point2D(Vector2 pos,float size,Vector4 Color,float depth = .5f) = 0;
		//Draw a line from position start to position end.The depth determines the covering order on the screen
		//Smaller the depth value the geometry will be more likely to cover other geometries
		//For exmaple a point which has depth value 0.5 will cover the point which has depth value 0.7
		//at the same place
		virtual void line2D(Vector2 start,Vector2 end,float width,Vector4 Color,float depth = .7f) = 0;

		//Set the view port of the 2D space.The center of the screen will be center value in world space
		//,the width of the view port rectangle will be caculated by the height and the screen's resolution
		virtual void set2DViewPort(Vector2 center,float height) = 0;

		//Draw a cricle at position 'pos',whose radius is 'radius' and color is 'Color',depth specifies the 
		//order of drawing.The powPoly specifies how many edges the polygon has to simulate the cricle(By default
		//it is pow(2,powPolygon = 4) = 16 )
		virtual void cricle2D(Vector2 pos, float radius, Vector4 Color, float depth = .8f, int powPoly = 4) = 0;

		//Draw a polygon,you should specify every point's position in the polygon
		//The points in the polygon must be in the order of clock wise
		//the polygon must be a convex polygon
		virtual void polygon2D(const Vector2* verts, int vertNum, Vector4 Color, float depth = .5f) = 0;

		//draw a loaded image onto the scene whose position is at center in world space.
		virtual void image2D(Texture& image,Vector2 center,Vector2 size,float rotate,float depth) = 0;

		//draw a loaded image onto the scene whose position is at center in world space.
		//draw multiple textures at one time
		virtual void image2D(Texture& image, Vector2* center,Vector2* size, float* rotate, float* depth,size_t num) = 0;

#ifdef _DEBUG
		//draw a single mesh object on the scene use the material.If the material is nullptr
		//then will use a default material.This api can only be used for testing.
		virtual void drawSingleMesh(Mesh& mesh,Material* mat = nullptr) = 0;

		//draw a single scene.This api can only be used for testing
		virtual void drawSingleScene(Scene* scene) = 0;
#endif
		//set a texture as the skybox.The texture must be a CUBE type
		//virtual void setSkyBox(Texture& sky) = 0;

		//set a camera as the main camera of the scene
		//virtual void setCamera(Camera& cam);
		
		//release all the meshs and buffer data in the scene
		virtual void releaseScene(Scene& scene) = 0;

		//upload some data in the scene to the rendering module
		virtual void uploadScene(Scene& scene) = 0;

		//parse drawcalls from outside and insert them into the command queue
		virtual void parseDrawCall(DrawCall* dc,int num) = 0;

		virtual void skybox(Texture& texture,SceneCamera* mainCamera) = 0;

		virtual bool parseShader(Shader* shader) = 0;

		virtual void resize() = 0;
	};
}