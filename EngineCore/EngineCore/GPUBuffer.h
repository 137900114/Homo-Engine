#pragma once
#include "Resource.h"

namespace  Core {
	class GPUBuffer : public  Resource {
	public:
		virtual void Create(UINT element_size,UINT element_num,void* init_data = nullptr);

		virtual void CreatePlaced(UINT element_size,UINT element_num, ID3D12Heap* heap,UINT heap_offset,
			void* init_data = nullptr);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() {

			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = mAddress;
			vbv.SizeInBytes = buffer_size;
			vbv.StrideInBytes = element_size;

			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() {
			D3D12_INDEX_BUFFER_VIEW ibv;

			ibv.BufferLocation = mAddress;
			ibv.Format = DXGI_FORMAT_R16_UINT;
			ibv.SizeInBytes = buffer_size;

			return ibv;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return UAVHandle; }
		const D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return SRVHandle; }

		const D3D12_GPU_VIRTUAL_ADDRESS  GetCBV() const { return mAddress; }
		const D3D12_GPU_VIRTUAL_ADDRESS  GetCBV(UINT index) const { return mAddress + index * element_size; }

		const UINT GetElementNum() const { return element_num; }

		void CreateSRVUAV(D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle);

		//不推荐使用的方法
		void CopyData2Buffer(void* data,ID3D12Resource** UploadBuffer,ID3D12GraphicsCommandList* cmdLis);

		GPUBuffer() {
			element_size = 0;
			element_num = 0;
			buffer_size = 0;

			UAVHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			SRVHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
		}

		~GPUBuffer() { Release(); }

	protected:

		D3D12_RESOURCE_DESC DescribeResource();

		UINT element_size;
		UINT element_num;
		UINT buffer_size;

		D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
	};
	
	
	class UploadBuffer : public GPUBuffer {
	public:
		void CreateSRVUAV(D3D12_CPU_DESCRIPTOR_HANDLE , D3D12_CPU_DESCRIPTOR_HANDLE ) = delete;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const = delete;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const = delete;

		virtual void Create(UINT element_size,UINT element_num,void* initial_data = nullptr) override;
		virtual void CreatePlaced(UINT element_size,UINT element_num,ID3D12Heap* heap,
			UINT heap_offset,void* initial_data = nullptr) override;

		void Upload2Buffer(void* src, UINT num, UINT indexOffset);

		~UploadBuffer() { UnMap(); Release(); }

	private:
		void* res_p;

		void Map() { mResource->Map(0, nullptr, &res_p); }
		void UnMap() { mResource->Unmap(0, nullptr); }

	};
	
	
}