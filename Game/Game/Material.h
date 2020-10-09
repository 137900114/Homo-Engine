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

		inline uint32_t GetOwnedConstantBufferSize() {
			return constantBufferCapability;
		}

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

		uint32_t constantBufferCapability;
		uint32_t constantBufferSize;
	};

	
}