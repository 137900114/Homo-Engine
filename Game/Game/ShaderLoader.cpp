#include "ShaderLoader.h"
#include "FileLoader.h"
#include "Buffer.h"
#include <vector>
#include "Common.h"

namespace Game {
	extern FileLoader* gFileLoader;
	
	class ShaderInclude : public ID3DInclude {
	private:
		std::vector<Buffer> buffers;
	public:
		virtual HRESULT _stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
		virtual HRESULT _stdcall Close(LPCVOID pData) override;
	};


	HRESULT _stdcall ShaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) {
		Buffer fileData;
		if (!gFileLoader->FileReadAndClose(pFileName, fileData)) {
			Log("shader loader fail to read file %s \n",pFileName);
			return E_FAIL;
		}

		buffers.push_back(std::move(fileData));
		*pBytes = fileData.size;
		*ppData = fileData.data;

		return S_OK;
	}


	HRESULT _stdcall ShaderInclude::Close(LPCVOID pData) {
		for (auto iter = buffers.begin(); iter != buffers.end(); iter++) {
			if (iter->data == pData) {
				iter->release();
				buffers.erase(iter);
				return S_OK;
			}
		}

		Log("shader loader fail to release file data at location %x\n",(uint64_t)pData);
		return E_FAIL;
	}

	ShaderInclude sInclude;

	void ShaderLoader::AddShaderIncludePath(const char* path) {
		gFileLoader->AddFileSearchPath(path);
	}
	
	ComPtr<ID3DBlob> ShaderLoader::CompileFile(const char* filename,const char* entry,
 		SHADER_TYPE type,const D3D_SHADER_MACRO* macros,UINT compileFlag) {
		ComPtr<ID3DBlob> target,errorCode;
		int i = 10;
		Buffer shaderData;
		if (!gFileLoader->FileReadAndClose(filename, shaderData)) {
			Log("shader loader fail to read file %s\n",filename);
			return nullptr;
		}

		HRESULT hr = D3DCompile(shaderData.data, shaderData.size,
			nullptr, macros, &sInclude, entry, target_list[type],
			compileFlag, 0, &target, &errorCode);

		if (FAILED(hr)) {
			Log("shader loader fail to compile shader file %s at entry %s",filename,entry);
			Log("\nreason : %s \n",errorCode->GetBufferPointer());
			return nullptr;
		}

		return target;
	}
}