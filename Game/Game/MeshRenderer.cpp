#include "MeshRenderer.h"
#include "GraphicModule.h"

namespace Game {
	extern MeshDrawCallMaker gMeshDrawCallMaker;
	MeshDrawCallMaker* MeshRenderer::drawCallMaker = &gMeshDrawCallMaker;

	extern GraphicModule* gGraphic;

	extern SceneManager gSceneManager;
}
 
Game::MeshRenderer::MeshRenderer(SceneObject* object,Material* mat): 
	Game::SceneComponent(object),object(object),material(mat){
	TransformBuffer.regID = 0;
	TransformBuffer.updated = true;
	TransformBuffer.data.resize(sizeof(ShaderObjectBuffer));
	
	ObjectBufferPtr = reinterpret_cast<ShaderObjectBuffer*>(TransformBuffer.data.data);
	active = true;
}

Game::MeshRenderer::~MeshRenderer() {
}

void Game::MeshRenderer::update(float /*deltatime*/) {
	Game::SceneTransform* trans = &object->transform;
	Mat4x4 world = trans->World;
	ObjectBufferPtr->world = world.T();
	ObjectBufferPtr->invWorld = world.R().T();
	
	TransformBuffer.updated = true;
}

void Game::MeshRenderer::initialize() {
	drawCallMaker->Register(this);
}

void Game::MeshRenderer::finalize() {
	drawCallMaker->UnRegister(this);
}

void Game::MeshDrawCallMaker::Register(Game::MeshRenderer* renderer) {
	//this->meshRenderers.push_back(renderer);
	Material* mat = renderer->material;
	DrawCall* dc = findDrawCall(mat);
	
	//the object will be rendered in this draw call
	if (dc != nullptr) {
		int index = dc->drawTargetNums++;
		dc->OwnedParameterBuffer.resize(dc->drawTargetNums * 3);
		dc->meshList.resize(dc->drawTargetNums);

		DrawCall::Parameter parameter;
		parameter.regID = 0;
		parameter.buffer = &renderer->TransformBuffer;
		DrawCall::MeshRef meshRef;
		meshRef.mesh = renderer->object->geoMesh;
		meshRef.activated = renderer->active;

		dc->OwnedParameterBuffer[index * 3] = parameter;
		parameter.regID = 1;
		dc->OwnedParameterBuffer[index * 3 + 1] = parameter;
		parameter.regID = 2;
		dc->OwnedParameterBuffer[index * 3 + 2] = parameter;
		dc->meshList[index] = meshRef;

		renderer->drawCallID = index;
	}
	else {
		DrawCall newDC;
		newDC.material = renderer->material;
		newDC.drawTargetNums = 1;
		
		Material* mat = newDC.material;
		newDC.parameterNums = 3;
		newDC.OwnedParameterBuffer.resize(3);
		newDC.meshList.resize(1);
		//currently we will only pass the object transform parameter to register 0
		//we will pass a camera parameter to register 1
		//we will pass a light parameter to register 2
		DrawCall::Parameter parameter;
		parameter.regID = 0;
		parameter.buffer = &renderer->TransformBuffer;
		DrawCall::MeshRef   meshRef;
		meshRef.mesh = renderer->object->geoMesh;
		meshRef.activated = renderer->active;

		newDC.meshList[0] = meshRef;
		newDC.OwnedParameterBuffer[0] = parameter;

		parameter.regID = 1;
		newDC.OwnedParameterBuffer[1] = parameter;
		parameter.regID = 2;
		newDC.OwnedParameterBuffer[2] = parameter;


		renderer->drawCallID = 0;
		drawCallList.push_back(newDC);
	}
}

Game::DrawCall* Game::MeshDrawCallMaker::findDrawCall(Game::Material* mat) {
	for (auto& item : drawCallList) {
		if (item.material == mat) {
			return &item;
		}
	}
	return nullptr;
}

bool Game::MeshDrawCallMaker::initialize() {
	return true;
}

void Game::MeshDrawCallMaker::finalize() {
}

void Game::MeshDrawCallMaker::UnRegister(Game::MeshRenderer* renderer) {
	DrawCall* dc = findDrawCall(renderer->material);
	if (dc) {
		dc->meshList[renderer->drawCallID].activated = false;
	}
}

void Game::MeshDrawCallMaker::tick() {
	SceneCamera* camera = gSceneManager.GetMainCamera();
	SceneLight* light = gSceneManager.GetMainLight();
	if (!camera || !light)
		return;
	
	//currently this two attributes is not in use
	UUID renderTargetID = camera->GetRenderTarget();
	UUID depthStencilID = camera->GetDepthStencil();


	for (int i = 0; i != drawCallList.size(); i++) {
		drawCallList[i].depthStencilBuffer = depthStencilID;
		drawCallList[i].renderTarget = renderTargetID;

		CBuffer* cameraBuffer = camera->GetCameraCBuffer();
		CBuffer* lightBuffer = light->GetLightCBuffer();
		uint32_t parameterNum = drawCallList[i].parameterNums;


		for (int j = 0; j != drawCallList[i].drawTargetNums; j++) {

			drawCallList[i].OwnedParameterBuffer[j * parameterNum + 1].buffer = cameraBuffer;
			//consider that every different object will have different light effecting them
			//the light parameter is a owned light
			drawCallList[i].OwnedParameterBuffer[j * parameterNum + 2].buffer = lightBuffer;
		}
	}

	gGraphic->parseDrawCall(drawCallList.data(), drawCallList.size());
}