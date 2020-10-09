#include "Shader.h"
#include "Common.h"
#include "Math.h"
using namespace Game;

ShaderParameter::ShaderParameter(std::string name, size_t offset, size_t padding_size, int regID, ShaderParameter::ShaderParameterType type) :name(name) {
	if (type >= ShaderParameterType::TEXTURE2D) {
		Log("ERROR:fail to create texture only constant parameter can be created by this constructor,creating parameter %s", name.c_str());
		this->attribute = INVAILD;
	}
	else {

		this->attribute = SHARED;
		this->offset = offset;
		this->regID = regID;
		this->padding_size = padding_size;

		//the constant buffer is 4 
		switch (type) {
		case ShaderParameterType::FLOAT:
		case ShaderParameterType::INT:
			this->size = 4;
			break;
		case ShaderParameterType::FLOAT2:
			this->size = sizeof(Vector2);
			break;

		case ShaderParameterType::FLOAT3:
			this->size = sizeof(Vector3);
			break;
		case ShaderParameterType::FLOAT4:
			this->size = sizeof(Vector4);
			break;
		case ShaderParameterType::FLOAT4X4:
			this->size = sizeof(Mat4x4);
			break;
		}
		this->type = type;
	}
}


ShaderParameter::ShaderParameter(std::string name, int regID, ShaderParameterType type) : name(name) {
	if (type >= ShaderParameter::TEXTURE2D && type <= ShaderParameter::TEXTURECUBE) {
		this->attribute = SHARED;
	}
	else {
		Log("ERROR:fail to create shader parameter,invaild shader parameter type,parameter name %s", name.c_str());
		this->attribute = INVAILD;
	}

	this->regID = regID;
	this->type = type;

}

ShaderParameter::ShaderParameter(std::string& name, ShaderParameterType type) :name(name) {
	if (type >= ShaderParameter::OWNED_INT) {
		switch (type) {
		case ShaderParameterType::OWNED_FLOAT:
		case ShaderParameterType::OWNED_INT:
			this->size = 4;
			break;
		case ShaderParameterType::OWNED_FLOAT2:
			this->size = sizeof(Vector2);
			break;

		case ShaderParameterType::OWNED_FLOAT3:
			this->size = sizeof(Vector3);
			break;
		case ShaderParameterType::OWNED_FLOAT4:
			this->size = sizeof(Vector4);
			break;
		case ShaderParameterType::OWNED_FLOAT4X4:
			this->size = sizeof(Mat4x4);
			break;
		}


		this->attribute = OWNED;
		this->regID = 0;
		this->type = type;

		this->padding_size = round_up(this->size, 16);
	}
	else {
		Log("ERROR:fail to create shader parameter,invaild shader parameter type,parameter name %s", name.c_str());
		this->attribute = INVAILD;
	}
}