#include "ShaderPraser.h"
#include "FileLoader.h"
#include "Common.h"
#include <sstream>
#include "GraphicModule.h"

namespace Game {
	extern FileLoader gFileLoader;
	extern GraphicModule* gGraphic;
}

using namespace Game;
using namespace std;

inline std::string shaderFullName(ShaderLanguageType language,std::string name) {
	switch (language) {
	case SHADER_LANGUAGE_HLSL:
		return "HLSL_" + name;
	//case SHADER_LANGUAGE_GLSL:
		//return "GLSL" + name;
	}
}

bool prasePipelineKeyWords(vector<string>& words,Shader* shader,std::string& error) {
	if (words.empty()) {
		error = "invaild pipeline attribute , a attributes is divided by ',' but on thing in it\n";
		return false;
	}

	if (words[0] == "FILL") {
		if (words.size() != 2) {
			error = "invaild pipeline attribute, the FILL attribute should be followed by a option (WIRE or SOLID)";
			return false;
		}

		if (words[1] == "WIRE") {
			shader->RasterizerState.fillMode = SHADER_FILL_MODE_WIREFRAME;
			return true;
		}
		else if (words[1] == "SOLID") {
			shader->RasterizerState.fillMode = SHADER_FILL_MODE_SOLID;
			return true;
		}
		else {
			error = "invalid pipeline attribute,invaild option for FILL attribute " + words[1];
			return false;
		}
	}

	if (words[0] == "CULL") {
		if (words.size() != 2) {
			error = "invalid pipeline attribute,the CULL attribute should be followed by a option(FRONT,BACK or NONE)";
			return false;
		}
		if (words[1] == "FRONT") {
			shader->RasterizerState.cullMode = SHADER_CULL_MODE_FRONT;
			return true;
		}
		else if (words[1] == "BACK") {
			shader->RasterizerState.cullMode = SHADER_CULL_MODE_BACK;
			return true;
		}
		else if(words[1] == "NONE"){
			shader->RasterizerState.cullMode = SHADER_CULL_MODE_NONE;
			return true;
		}
		else {
			error = "invalid pipeline attribute,invalid option for CULL attribute " + words[1];
			return false;
		}
	}

	auto get_blend_factor = [](std::string& name,ShaderBlendState & factor,bool isSrc)->bool {
		if (name == "ONE") {
			factor = SHADER_BLEND_ONE;
			return true;
		}
		else if (name == "ZERO") {
			factor = SHADER_BLEND_ZERO;
			return true;
		}
		else if (name == "ALPHA") {
			factor = isSrc ? SHADER_BLEND_SRC_ALPHA : SHADER_BLEND_DEST_ALPHA;
			return true;
		}
		else if (name == "INVALPHA") {
			factor = isSrc ? SHADER_BLEND_INV_SRC_ALPHA : SHADER_BLEND_INV_DEST_ALPHA;
			return true;
		}

		return false;
	};

	auto get_blend_operation = [](std::string& name,ShaderBlendOperation& op)->bool {
		if (name == "ADD") {
			op = SHADER_BLEND_OP_ADD;
			return true;
		}
		else if (name == "SUB") {
			op = SHADER_BLEND_OP_SUBTRACT;
			return true;
		}
		else if (name == "MAX") {
			op = SHADER_BLEND_OP_MAX;
			return true;
		}
		else if (name == "MIN") {
			op = SHADER_BLEND_OP_MIN;
			return true;
		}

		return false;
	};

	if (words[0] == "BLEND") {
		if (words.size() != 3 || words.size() != 4) {
			error = "invaild pipeline attribute,the BLEND attribute have at least two options";
			return false;
		}
		ShaderBlendState Src, Dst;
		if (!get_blend_factor(words[1],Src,true)
			|| !get_blend_factor(words[2],Dst,false)) {
			error = "invalid pipeline attribute, the BLEND attribute encountered invaild blend options,the source is " + words[1]
				+ "the destination is " + words[2];
			return false;
		}
		shader->BlendDesc.blendSrc = Src;
		shader->BlendDesc.blendDest = Dst;

		if (words.size() == 4) {
			ShaderBlendOperation Opera;
			if (!get_blend_operation(words[3],Opera)) {
				error = "invalid pipeline attribute,the BLEND attribute invalid operation name " + words[3];
				return false;
			}
			shader->BlendDesc.blendOp = Opera;
		}
	}

	if (words[0] == "BLEND_ALPHA") {
		if (words.size() != 3 || words.size() != 4) {
			error = "invalid pipeline attribute,the BLEND attribute have at least two options";
			return false;
		}
		ShaderBlendState Src, Dst;
		if (!get_blend_factor(words[1],Src,true)
			|| !get_blend_factor(words[2],Dst,false)) {
			error = "invalid pipeline attribute,the BLEND_ALPHA attribute encountered invalid options,the source is " + words[1]
				+ "the destination is " + words[2];
			return false;
		}
		shader->BlendDesc.blendAlphaSrc = Src;
		shader->BlendDesc.blendAlphaDest = Dst;

		if (words.size() == 4) {
			ShaderBlendOperation Opera;
			if (!get_blend_operation(words[3],Opera)) {
				error = "invalid pipeline attribute,the BLEND_ALPHA attribute invalid operation name " + words[3];
				return false;
			}
			shader->BlendDesc.blendAlphaOp = Opera;
		}
	}

	error = "invalid pipeline attribute " + words[0];
	return false;
}

enum ShaderCodeBlockType {
	SHADER_CODE_TYPE_DEFINE,
	SHADER_CODE_TYPE_VS,
	SHADER_CODE_TYPE_PS
};

struct ShaderCodeBlock {
	std::string code;
	ShaderCodeBlockType type;
};


bool hlslPraser(std::vector<ShaderCodeBlock>& code_blocks,std::string& vertexOut,Shader* shader,std::string& error) {
	return false;
}

struct ShaderCodeBlockParameter {
	std::string name;
	std::string value;
};

bool getCodeBlockParameters(std::istringstream& stream,std::vector<ShaderCodeBlockParameter>& parameters) {
	
	std::string str1, op, str2;
	stream >> str1;
	if (str1 != "[") {
		return true;
	}

	while (!stream.eof()) {
		stream >> str1 >> op >> str2;
		parameters.push_back(ShaderCodeBlockParameter{ str1,str2 });
		stream >> op;
		if (op == "]") {
			return true;
		}
	}
	return false;
}

bool getCodeBlock(istringstream& stream, std::string& buffer, std::string& error) {
	std::string word;
	stream >> word;
	if (word != "{") {
		error = "code blocks should be start with a '{'";
		return false;
	}
	std::string line;
	while (std::getline(stream, line)) {
		if (line == "}") {
			return true;
		}
		else {
			buffer += line + "\n";
		}
	}

	error = "code blocks should be ended with a '}'";
	return false;
}

bool getPipelineState(istringstream& stream, Shader* shader, std::string& error) {
	std::string brackets;
	stream >> brackets;
	if (brackets != "[") {
		error = "invaild expression,the pipeline state descriptors should start with a '['";
		return false;
	}

	vector<std::string> words;
	while (!stream.eof()) {
		std::string currentWord;
		stream >> currentWord;
		if (currentWord == "]") {
			break;
		}
		else if (currentWord == ",") {
			if (!prasePipelineKeyWords(words, shader, error)) {
				return false;
			}
			words.clear();
		}
		else {
			words.push_back(currentWord);
		}

	}

	return true;
}

Shader* ShaderParser::prase(ShaderLanguageType language,const char* filename) {
	Buffer data;
	
	std::string fullFilename = shaderFullName(language,filename);
	if (Shader* shader = getShaderByFileName(fullFilename)) {
		Log("ShaderPraser : the shader %s have been loaded\n",shader->fileName.c_str());
		return shader;
	}

	if (!gFileLoader.FileReadAndClose(filename, data)) {
		Log("ShaderPraser : fail to read shader data from file %s", filename);
		return nullptr;
	}

	istringstream istream(reinterpret_cast<const char*>(data.data),ios_base::beg);

	std::string shaderName,VertexOut;
	Shader newShader("");
	std::vector<ShaderCodeBlock> codeBlocks;

	while (!istream.eof()) {
		std::string word;
		istream >> word;

		if (word == "NAME") {
			std::string trash,name;
			istream >> trash >> name;
			shaderName = name;
		}
		else if (word == "PIPELINE") {
			std::string error;
			if (!getPipelineState(istream,&newShader,error)) {
				Log("Fail to prase PIPELINE states error message : %s",error.c_str());
				return nullptr;
			}
		}
		else if (word == "VS") {
			std::string trash,buffer,error;
			if (!getCodeBlock(istream,buffer,error)) {
				Log("Fail to prase VS code block error message : %s",error.c_str());
				return nullptr;
			}
			codeBlocks.push_back(ShaderCodeBlock{ buffer,SHADER_CODE_TYPE_VS });
		}
		else if (word == "PS") {
			std::string trash, buffer, error;
			istream >> trash;
			if (!getCodeBlock(istream,buffer,error)) {
				Log("Fail to prase PS code block error message : %s",error.c_str());
				return nullptr;
			}
			codeBlocks.push_back(ShaderCodeBlock{ buffer,SHADER_CODE_TYPE_PS });
		}
		else if (word == "DEFINE") {
			std::vector<ShaderCodeBlockParameter> parameters;
			std::string trash, buffer, error;
			istream >> trash >> VertexOut;
			if (!getCodeBlockParameters(istream,parameters)) {
				Log("Fail to prase DEFINE code block , fail to prase the parameters\n");
				return nullptr;
			}

			for (auto item : parameters) {
				if (item.name == "VOUT") {
					VertexOut = item.value;
				}
			}
			if (!getCodeBlock(istream,buffer,error)) {
				Log("Fail to prase DEFINE code block error message : %s\n",error.c_str());
				return nullptr;
			}
			codeBlocks.push_back(ShaderCodeBlock{ buffer,SHADER_CODE_TYPE_DEFINE });
		}
	}

	std::string error;
	if (!hlslPraser(codeBlocks, VertexOut, &newShader,error)) {
		Log("fail to prase code\n");
		return nullptr;
	}

	if (getShaderByName(shaderName) != nullptr) {
		Log("fail to create shader the shader has repeated name %s",shaderName.c_str());
		return nullptr;
	}

	newShader.name = shaderName;
	newShader.pipelineStateObject = shaderName;
	shaders[shaderName] = newShader;
	
	switch (language) {
	case SHADER_LANGUAGE_HLSL:
		if (!hlslPraser(codeBlocks,VertexOut,getShaderByName(shaderName),error)) {
			Log("fail to prase the shader code to hlsl code,reason : %s",error.c_str());
			return nullptr;
		}
	default:
		Log("the shader language is not supported\n");
		return nullptr;
	}

	return getShaderByName(shaderName);
	
}