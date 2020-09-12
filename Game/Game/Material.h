#pragma once
#include "Memory.h"
#include <string>
#include "Math.h"
#include "uuid.h"
#include <map>
#include "Texture.h"
#include "Shader.h"


namespace Game {

	struct CBuffer {
		Buffer data;
		UUID uid;//the id of the cbuffer in the cbuffer
		int regID;
		bool updated;
	};

	struct TBuffer {
		Texture* texture;
		int regID;
	};

	struct ShaderParameter {
		enum ShaderParameterType {
			INT = 0,//a 32 bit int number
			FLOAT = 1,//a 32 bit float number
			FLOAT2 = 2,//a 2 dimension 32 bit float vector
			FLOAT3 = 3,//a 3 dimension 32 bit float vector
			FLOAT4 = 4,//a 4 dimension 32 bit float vector
			FLOAT4X4 = 5,//a 4x4 dimension 32 bit float matrix
			TEXTURE2D = 0x10,//a dimension texture
			TEXTURECUBE = 0x11,// a cube texture

			OWNED_CBUFFER = 0x20//this parameter will be binded to an other cbuffer
		} type;

		enum ShaderParameterAttribute {
			SHARED,//the Shader parameter is shadered among all the objects
			OWNED,//the Shader parameter belong to one object.one object may have a different value to one other object 
			INVAILD
		} attribute;

		std::string name;
		size_t offset;//the offset in the buffer for constants
		size_t padding_size;
		size_t size;
		//the register will be used for constant buffer or texture buffer
		//if the register id is -1 system will allocate a new reg id
		int regID;

		ShaderParameter(): attribute(INVAILD) {

		}


		//a shared constant shader parameter constructor
		ShaderParameter(std::string name, size_t offset,size_t padding_size,int regID, ShaderParameterType SPType);
		//a owned shader parameter constructor or shared texture parameter constructor
		ShaderParameter(std::string name, int regID,ShaderParameterType SPType);
	};

	class Material {
		friend class MeshDrawCallMaker;
	public:

		//only shared parameters can be queryed and changed by material
		inline bool QueryInt(std::string& name, int* value) {
			return QueryData(name, value, ShaderParameter::INT);
		}
		inline bool QueryFloat(std::string& name, float* value) {
			return QueryData(name, value, ShaderParameter::FLOAT);
		}
		inline bool QueryVector2(std::string& name, Vector2* value) {
			return QueryData(name, value, ShaderParameter::FLOAT2);
		}
		inline bool QueryVector3(std::string& name, Vector3* value) {
			return QueryData(name, value, ShaderParameter::FLOAT3);
		}
		inline bool QueryVector4(std::string& name, Vector4* value) {
			return QueryData(name, value, ShaderParameter::FLOAT4);
		}

		bool QueryTexture2D(std::string& name, Texture** texture);
		bool QueryTextureCube(std::string& name, Texture** texture);

		inline bool SetInt(std::string& name, int value) {
			return SetData(name, &value, ShaderParameter::INT);
		}
		inline bool SetFloat(std::string& name, float value) {
			return SetData(name, &value, ShaderParameter::FLOAT);
		}
		inline bool SetFloat2(std::string& name, Vector2 value) {
			return SetData(name,&value,ShaderParameter::FLOAT2);
		}
		inline bool SetFloat3(std::string& name, Vector3 value) {
			return SetData(name,&value,ShaderParameter::FLOAT3);
		}
		inline bool SetFloat4(std::string& name, Vector4 value) {
			return SetData(name,&value,ShaderParameter::FLOAT4);
		}

		bool SetTexture2D(std::string& name  ,Texture* value);
		bool SetTextureCube(std::string& name,Texture* value);

		
		bool SetSharedCBuffer(int regID,void* data,size_t size);

		Material(ShaderParameter* SPArray, size_t SPNum,std::string name,Shader* shader = nullptr);

		std::vector<CBuffer>& GetSharedConstantBuffers() {
			return shared_cbuffers;
		}
		std::vector<TBuffer>& GetSharedTextureBuffers() {
			return shared_tbuffers;
		}

		Shader* GetShader() {
			return shader;
		}
		
	private:
		CBuffer* find_shared_cbuffer(int regID) {
			for (auto& item : shared_cbuffers) {
				if (item.regID == regID) {
					return &item; 
				}
			}
			return nullptr;
		}

		TBuffer* find_shared_tbuffer(int regID) {
			for (auto& item : shared_tbuffers) {
				if (item.regID == regID) {
					return &item;
				}
			}
			return nullptr;
		}

		void write_to_cbuffer(size_t offset, size_t size, void* value, CBuffer* buffer);

		void read_from_cbuffer(size_t offset, size_t size, void* value, CBuffer* buffer) {
			memcpy(value, buffer->data.data + offset, size);
		}

		bool QueryData(std::string& name,void* value,ShaderParameter::ShaderParameterType type);
		bool SetData(std::string& name,void* value,ShaderParameter::ShaderParameterType type);

		std::map<std::string, ShaderParameter> shared_parameters;
		std::map<std::string, ShaderParameter> owned_parameters;
		std::vector<CBuffer> shared_cbuffers;
		std::vector<TBuffer> shared_tbuffers;
		int current_max_reg_idc = 0;
		int current_max_reg_idt = 0;
		std::string name;
		Shader* shader;
	};

	
}