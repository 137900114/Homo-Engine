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
	ConstantBuffer.regID = 0;
	ConstantBuffer.updated = true;
	if(mat != nullptr)
		ConstantBuffer.data.resize(mat->GetOwnedConstantBufferSize());
	else 	
		ConstantBuffer.data.resize(sizeof(ShaderObjectBuffer));
	ObjectBufferPtr = reinterpret_cast<ShaderObjectBuffer*>(ConstantBuffer.data.data);
	active = true;
}

Game::MeshRenderer::~MeshRenderer() {
}

void Game::MeshRenderer::update(float /*deltatime*/) {
	Game::SceneTransform* trans = &object->transform;
	Mat4x4 world = trans->GetWorld();
	ObjectBufferPtr->world = world.T();
	ObjectBufferPtr->invWorld = world.R().T();
	
	ConstantBuffer.updated = true;
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
		

		DrawCall::Parameter parameter;
		parameter.objectBuffer = &renderer->ConstantBuffer;

		DrawCall::MeshRef meshRef;
		meshRef.mesh = renderer->object->geoMesh;
		meshRef.activated = true;

		dc->OwnedParameterBuffer.push_back(parameter);
		dc->meshList.push_back(meshRef);

		renderer->drawCallID = index;
	}
	else {
		DrawCall newDC;
		newDC.material = nullptr;
		newDC.drawTargetNums = 1;

		//currently we will only pass the object transform parameter to register 0
		//we will pass a camera parameter to register 1
		//we will pass a light parameter to register 2
		DrawCall::Parameter parameter;
		parameter.objectBuffer = &renderer->ConstantBuffer;
		DrawCall::MeshRef   meshRef;
		meshRef.mesh = renderer->object->geoMesh;
		meshRef.activated = renderer->active;

		newDC.meshList.push_back(meshRef);
		newDC.OwnedParameterBuffer.push_back(parameter);

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
		if (drawCallList[i].material != nullptr) {
			continue;
		}

		drawCallList[i].depthStencilBuffer = depthStencilID;
		drawCallList[i].renderTarget = renderTargetID;

		CBuffer* cameraBuffer = camera->GetCameraCBuffer();
		CBuffer* lightBuffer = light->GetLightCBuffer();
		uint32_t parameterNum = drawCallList[i].parameterNums;


		for (int j = 0; j != drawCallList[i].drawTargetNums; j++) {

			drawCallList[i].OwnedParameterBuffer[j].cameraBuffer = cameraBuffer;
			//consider that every different object will have different light effecting them
			//the light parameter is a owned light
			drawCallList[i].OwnedParameterBuffer[j].lightBuffer = lightBuffer;
		}
	}

	gGraphic->parseDrawCall(drawCallList.data(), drawCallList.size());
}