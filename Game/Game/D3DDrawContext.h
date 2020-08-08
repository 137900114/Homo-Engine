#pragma once
#include "d3dCommon.h"
#include "Common.h"

namespace Game {

	struct ShaderParameter{
			D3D12_ROOT_PARAMETER_TYPE type;
			union {
				D3D12_GPU_VIRTUAL_ADDRESS address;
				D3D12_GPU_DESCRIPTOR_HANDLE handle;
				struct {
					void* data;
					uint32_t size;
				} constant32Bit;
			}Data;
			uint32_t parameterIndex;
			ShaderParameter() {

			}

			ShaderParameter(D3D12_GPU_VIRTUAL_ADDRESS gva,D3D12_ROOT_PARAMETER_TYPE type,uint32_t parameterIndex):
				type(type),parameterIndex(parameterIndex){
				Data.address = gva;
			}

			ShaderParameter(D3D12_GPU_DESCRIPTOR_HANDLE handle, D3D12_ROOT_PARAMETER_TYPE type, uint32_t parameterIndex):
				type(type),parameterIndex(parameterIndex){
				Data.handle = handle;
			}

			void DoCopy(const ShaderParameter& other) {
				type = other.type;
				switch (type) {
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
					Data.address = other.Data.address;
					break;
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					Data.handle = other.Data.handle;
					break;
				}

				parameterIndex = other.parameterIndex;
			}

			ShaderParameter& operator=(const ShaderParameter& other) {
				DoCopy(other);
				return *this;
			}

			ShaderParameter(const ShaderParameter& other) {
				DoCopy(other);
			}


	};

	class D3DDrawContext {
	public:
		inline bool SetShaderParameter(uint32_t index,ShaderParameter shaderParameter) {
			if (index > parameterNum) {
				return false;
			}
			parameters[index] = shaderParameter;
			return true;
		}

		void BindOnCommandList(ID3D12GraphicsCommandList* mCmdList);
		
		~D3DDrawContext();

		D3DDrawContext(ID3D12PipelineState* pso,ID3D12RootSignature* rootSig,
						ID3D12DescriptorHeap* descHeap,
						uint32_t parameterNum, D3D12_VERTEX_BUFFER_VIEW vbv, uint32_t vertNum
						,uint32_t vertBaseIndex = 0,
			            D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
						uint32_t instanceNum = 1);

		D3DDrawContext(ID3D12PipelineState* pso,ID3D12RootSignature* rootSig,
						ID3D12DescriptorHeap* descHeap,
						uint32_t parameterNum,D3D12_VERTEX_BUFFER_VIEW vbv,uint32_t vertNum,
						D3D12_INDEX_BUFFER_VIEW ibv,uint32_t indexNum,uint32_t vertBaseIndex = 0,
						uint32_t indexBaseIndex = 0,D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
						uint32_t instanceNum = 1);

	private:
		ComPtr<ID3D12PipelineState> pso;
		ComPtr<ID3D12RootSignature> rootSig;
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_VERTEX_BUFFER_VIEW vbv;
		uint32_t vertexBaseIndex;
		uint32_t vertNum;

		bool isIndexed;
		D3D12_INDEX_BUFFER_VIEW ibv;
		uint32_t indexNum;
		uint32_t indexBaseIndex;

		ShaderParameter*  parameters;
		uint32_t parameterNum;

		D3D_PRIMITIVE_TOPOLOGY topology;

		bool drawInstanced;
		uint32_t instanceNum;
	};


}

