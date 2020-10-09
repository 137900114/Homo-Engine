#pragma once
#include "Material.h"
#include "Mesh.h"
#include "MArray.h"
#include <vector>


namespace Game {
	struct DrawCall {
		
		union  Parameter{
			struct {
				CBuffer* objectBuffer;//the corresponding register slot is 0
				CBuffer* cameraBuffer;//the corresponding register slot is 1
				CBuffer* lightBuffer;//the corresponding register slot is 2 if the buffer is nullptr which means that we don't need light.
			};
			CBuffer* buffers[3];
		};

		struct MeshRef {
			bool activated;
			Mesh* mesh;
		};

		uint32_t  drawTargetNums;
		uint32_t  parameterNums;

		Material* material;
		std::vector<Parameter>    OwnedParameterBuffer;
		std::vector<MeshRef>	     meshList;
		UUID      renderTarget;
		UUID      depthStencilBuffer;
	};
}