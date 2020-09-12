#include "Material.h"
#include "Common.h"

namespace Game {

}

using namespace Game;

void Material::write_to_cbuffer(size_t offset, size_t size, void* value, CBuffer* buffer) {
	if (buffer->data.size < offset + size) {
		Buffer temp = buffer->data;

		size_t new_buffer_size = round_up(offset + size , 256);
		buffer->data.resize(new_buffer_size);
		memcpy(buffer->data.data, temp.data, temp.size);
	}
	memcpy(buffer->data.data + offset, value, size);
	buffer->updated = true;
}


Material::Material(ShaderParameter* SPArray,size_t SPNum,std::string name,Shader* shader) : name(name),shader(shader){

	int maxIndexNum = 0;
	for (int i = 0; i != SPNum; i++) {
		ShaderParameter SP = SPArray[i];
		if (shared_parameters.find(SP.name) != shared_parameters.end()
			|| owned_parameters.find(SP.name) != owned_parameters.end()) {
			Log("ERROR:the shader parameters shouldn't have the same name,name collision on %s this parameter will be skipped"
				" while create material %s\n",SP.name.c_str(),name.c_str());
			continue;
		}
		if (SP.attribute == ShaderParameter::SHARED) {
			if (SP.type <= ShaderParameter::FLOAT4X4) {
				CBuffer* buffer = find_shared_cbuffer(SP.regID);
				if (!buffer) {
					CBuffer newbuffer;
					newbuffer.data.resize(round_up(SP.padding_size + SP.offset, 256));
					newbuffer.regID = SP.regID;
					newbuffer.updated = false;

					shared_cbuffers.push_back(std::move(newbuffer));
					buffer = &shared_cbuffers[shared_cbuffers.size() - 1];
				}
				if (buffer->data.size < SP.padding_size + SP.offset) {
					buffer->data.resize(round_up(SP.padding_size + SP.offset, 256));
				}
			}
			else if (SP.type <= ShaderParameter::TEXTURECUBE) {
				TBuffer* buffer = find_shared_tbuffer(SP.regID);
				if (!buffer) {
					TBuffer newBuffer;
					newBuffer.regID = SP.regID;
					
					shared_tbuffers.push_back(std::move(newBuffer));
				}
				else {
					Log("ERROR: the texture register %d is occupied,parameter %s will be skipped",SP.regID,SP.name.c_str());
					continue;
				}
			}
			else if (SP.type >= ShaderParameter::OWNED_CBUFFER) {
				Log("ERROR:invaild attribute type for shader parameter if the shader parameter is a owned attribute "
						"then the attribute type of the shader parameter should be owned,error parameter %s"
					" while creating the material %s\n",SP.name.c_str(),name.c_str());
				continue;
			}
			shared_parameters[SP.name] = SP;
		}
		else if(SP.attribute == ShaderParameter::OWNED){
			if (SP.type < ShaderParameter::OWNED_CBUFFER) {
				Log("ERROR:invaild attribute type for shader parameter if the shader parameter is a shared attribute "
					"then the attribute type of the shader parameter should be shared %s"
					"while creating material %s\n",SP.name.c_str(),name.c_str());
				continue;
			}
			owned_parameters[SP.name] = SP;
		}
		else if (SP.attribute == ShaderParameter::INVAILD) {
			Log("ERROR:invaild shader parameter %s,while creating material %s\n",SP.name.c_str(),name.c_str());
			continue;
		}
	}
}

ShaderParameter::ShaderParameter(std::string name,size_t offset,size_t padding_size,int regID,ShaderParameter::ShaderParameterType type):name(name) {
	if (type >= ShaderParameterType::TEXTURE2D) {
		Log("ERROR:fail to create texture only constant parameter can be created by this constructor,creating parameter %s" , name.c_str());
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


ShaderParameter::ShaderParameter(std::string name,int regID,ShaderParameterType type ): name(name) {
	if (type >= ShaderParameterType::OWNED_CBUFFER) {
		this->attribute = OWNED;
		
	}
	else if(type >= ShaderParameter::TEXTURE2D){
		this->attribute = SHARED;
	}
	else {
		Log("ERROR:fail to create shader parameter,invaild shader parameter type,parameter name %s",name.c_str());
		this->attribute = INVAILD;
	}

	this->regID = regID;
	this->type = type;
	
}

bool Material::QueryData(std::string& name,void* value,ShaderParameter::ShaderParameterType type) {
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end()
		|| query->second.type != type) {
		return false;
	}

	ShaderParameter* parameter = &query->second;
	CBuffer* buffer = find_shared_cbuffer(parameter->regID);
	if (!buffer) return false;
	read_from_cbuffer(parameter->offset, parameter->size, value, buffer);

	return true;
}


bool Material::QueryTexture2D(std::string& name, Texture** value) {
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end() ||
		query->second.type != ShaderParameter::TEXTURE2D) {
		return false;
	}

	TBuffer* buffer = find_shared_tbuffer(query->second.regID);
	if (!buffer) return false;
	*value = buffer->texture;
	return true;
}

bool Material::QueryTextureCube(std::string& name,Texture** value){
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end()||
		query->second.type != ShaderParameter::TEXTURECUBE) {
		return false;
	}

	TBuffer* buffer = find_shared_tbuffer(query->second.regID);
	if (!buffer) return false;
	*value = buffer->texture;
	return true;
}


bool Material::SetData(std::string& name,void* data,ShaderParameter::ShaderParameterType type) {
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end()||
		query->second.type != type) {
		return false;
	}
	
	ShaderParameter* parameter = &query->second;
	CBuffer* buffer = find_shared_cbuffer(parameter->regID);
	if (!buffer) return false;

	write_to_cbuffer(parameter->offset, parameter->size, data, buffer); 
	buffer->updated = true;
	return true;
}

bool Material::SetTexture2D(std::string& name,Texture* value) {
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end() ||
		query->second.type != ShaderParameter::TEXTURE2D ||
		value->type != TEXTURE_TYPE::TEXTURE2D) {
		return false;
	}

	ShaderParameter* parameter = &query->second;

	TBuffer* buffer = find_shared_tbuffer(parameter->regID);
	if (!buffer) return false;
	buffer->texture = value;
	return true;
}

bool Material::SetTextureCube(std::string& name, Texture* value) {
	auto query = shared_parameters.find(name);
	if (query == shared_parameters.end() ||
		query->second.type != ShaderParameter::TEXTURECUBE ||
		value->type != TEXTURE_TYPE::CUBE) {
		return false;
	}

	ShaderParameter* parameter = &query->second;

	TBuffer* buffer = find_shared_tbuffer(parameter->regID);
	if (!buffer) return false;
	buffer->texture = value;
	return true;
}

bool Material::SetSharedCBuffer(int regID,void* data,size_t size) {

	CBuffer* buffer = find_shared_cbuffer(regID);
	if (!buffer || buffer->data.size < size) return false;
	write_to_cbuffer(0, size, data, buffer);
	return true;
}