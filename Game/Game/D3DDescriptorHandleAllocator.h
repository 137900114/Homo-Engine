#pragma once
#include "d3dCommon.h"
#include "uuid.h"
#include <map>

namespace Game {

	struct D3DDescriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		//size_t offset;

		static const D3DDescriptorHandle invalid;

		bool operator==(const D3DDescriptorHandle& handle) {
			return gpuHandle.ptr == handle.gpuHandle.ptr
				&& cpuHandle.ptr == handle.cpuHandle.ptr;
		}
	};

	class D3DDescriptorHandleAllocator {
	public:
		//if the resource haven't been uploaded to descriptor table yet.Allocator will upload it to 
		//the descriptor table and return the corresponding handler.The handle will change dynamicly
		//,so rather than save the handler,you should query the handler every frame.
		D3DDescriptorHandle getDescriptorHandle(UUID id,D3D12_DESCRIPTOR_HEAP_TYPE type);

		D3DDescriptorHandle AllocateNewHandle(UUID id,D3D12_DESCRIPTOR_HEAP_TYPE type);

		inline ID3D12DescriptorHeap* getDescriptorHeap() {
			return mCurrSRVUAVHeap.Get();
		}

		bool initiailize(ID3D12Device* dev);
	private:
		ComPtr<ID3D12DescriptorHeap> createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
														  size_t size);

		std::map<UUID, D3DDescriptorHandle> handles;
		ComPtr<ID3D12DescriptorHeap> mCurrSRVUAVHeap;
		size_t mCurrSRVUAVHeapSize,mCurrSRVUAVHeapOffset;
		size_t mSRVUAVHandleSize;
		

		ID3D12Device* device;
	};
}