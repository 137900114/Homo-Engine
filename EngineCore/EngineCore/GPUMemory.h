#pragma once
#include "common.h"
#include "GPUBuffer.h"

namespace Core {

	struct GPUMemAlloc {
		void* CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
		Resource& mResource;
		UINT Size;
		UINT Offset;

		GPUMemAlloc(Resource& res,UINT Size,UINT Offset):
		mResource(res),GPUAddress(res.GetVirtualAddress() + Offset),
		Size(Size),Offset(Offset)
		{ 
			res.Get()->Map(0,nullptr,&CPUPtr);
			CPUPtr = static_cast<uint8_t*>(CPUPtr) + Offset;
		}

		~GPUMemAlloc() {
			//mResource.Get()->Unmap(0, nullptr);
			//为什么会不需要Unmap?
		}
	};

	class UploadMemAllocator {
	public:
		UploadMemAllocator():
		mCurrentPage(CreateNewResource()),
		mCurrentOffset(0){
			MemPool.push_back(std::unique_ptr<Resource>(mCurrentPage));
		}

		GPUMemAlloc Allocate(UINT size);

		void Release(FenceVal fence);

		void ReleaseAll();

		~UploadMemAllocator() { ReleaseAll(); }
	private:
		void FlushUsedMem(FenceVal fence);

		Resource* CreateNewResource(UINT Size = mResourceSize);

		std::vector<std::unique_ptr<Resource>> MemPool;
		std::queue<std::pair<FenceVal, Resource*>> UsedLargePages;
		std::queue < std::pair <FenceVal, Resource*>> UsedMem;
		std::queue<Resource*> AvaliableMem;
		std::queue<std::pair<bool, Resource*>> ToReleasePages;

		Resource* mCurrentPage;
		UINT mCurrentOffset;

		static const UINT mResourceSize;
	};
}