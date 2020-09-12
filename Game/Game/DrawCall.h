#pragma once
#include "Material.h"
#include "Mesh.h"
#include "MArray.h"


namespace Game {
	struct DrawCall {
		struct Parameter{
			uint32_t regID;
			CBuffer* buffer;
		};

		struct MeshRef {
			bool activated;
			Mesh* mesh;
		};

		uint32_t  drawTargetNums;
		uint32_t  parameterNums;

		Material* material;
		MArray<Parameter>    OwnedParameterBuffer;
		MArray<MeshRef>	     meshList;
		UUID      renderTarget;
		UUID      depthStencilBuffer;
	};
}