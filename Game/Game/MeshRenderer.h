#pragma once
//making draw calls that render the objects in the scene
#include "IRuntimeModule.hpp"
#include "SceneComponent.h"
#include "SceneObject.h"
#include "DrawCall.h"
#include "ShaderDataStructure.h"
#include <vector>


namespace Game{
	class MeshDrawCallMaker;

	enum RENDER_PIPELINE_TYPE {
		DEFAULT
	};

	class MeshRenderer : public SceneComponent {
		friend class MeshDrawCallMaker;
	public:
		MeshRenderer(SceneObject* object,Material* mat = nullptr);

		virtual void update(float deltatime) override;
		virtual void initialize() override;
		virtual void finalize() override;
		virtual size_t componentSize() override {
			return sizeof(MeshRenderer);
		}

		~MeshRenderer();
	private:
		
		
		static MeshDrawCallMaker* drawCallMaker;
		SceneObject* object;
		Material* material;
		CBuffer TransformBuffer;
		ShaderObjectBuffer* ObjectBufferPtr;
		uint32_t drawCallID;
	};


	class MeshDrawCallMaker : public IRuntimeModule {
	public:
		//register a mesh renderer to the draw call maker so that 
		void Register(MeshRenderer* renderer);
		void UnRegister(MeshRenderer* renderer);

		virtual bool initialize() override;
		virtual void tick() override;
		virtual void finalize() override;

	private:
		DrawCall* findDrawCall(Material* mat);

		std::vector<DrawCall> drawCallList;
		//std::vector<MeshRenderer*> meshRenderers;
	};
}