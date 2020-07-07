#pragma once
#include "d3dCommon.h"
#include <d3dcompiler.h>


namespace Game {
	enum SHADER_TYPE {
		VS = 0, PS = 1
	};


	class ShaderLoader {
	public:
		//insert a shader include path into the fileloader search paths.
		void AddShaderIncludePath(const char* path);
		ComPtr<ID3DBlob> CompileFile(const char* fileLocation,const char* entry,SHADER_TYPE type,
			const D3D_SHADER_MACRO* macros = nullptr,UINT compileFlag = 0);
	private:
		const char* target_list[2] = {"vs_5_0","ps_5_0"};
		
	};

}