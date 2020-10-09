#pragma once
#include "Shader.h"
#include "IRuntimeModule.hpp"
#include "Math.h"
#include "DrawCall.h"
#include "Mesh.h"


namespace Game {
	
	class Debug : public IRuntimeModule{
	public:

		void log(const char* str);

		template<typename ...T>
		void log(const char* format,T ...args);

		void RayCast(Vector3 start,Vector3 end,Color color = ConstColor::Black);
		


		virtual void tick() override;
		virtual bool initialize() override;
		virtual void finalize() override;
	private:
		struct ObjectConstantBuffer {
			Mat4x4 world;
			Mat4x4 invWorld;
			Vector4 color;
		};


		std::vector<DrawCall> drawCallList;
		
		Material* DebugMat;

		Mesh cubeMesh;
		Mesh sphereMesh;
		Mesh planeMesh;

	};

}