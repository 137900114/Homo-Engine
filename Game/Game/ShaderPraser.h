#pragma once
#include "Shader.h"
#include <map>

namespace Game {

	//shader parser will compile shader in hmsl language into hlsl or glsl.
	class ShaderParser {
	public:
		Shader* prase(ShaderLanguageType type,const char* filename);
		Shader* getShaderByName(std::string& name) {
			auto iter = shaders.find(name);
			if (iter != shaders.end()) {
				return &iter->second;
			}
			return nullptr;
		}
		Shader* getShaderByFileName(std::string& filename) {
			for (auto iter : shaders) {
				if (iter.second.name == filename) {
					return &iter.second;
				}
			}
			return nullptr;
		}
	private:
		std::map<std::string,Shader> shaders;
	};
}