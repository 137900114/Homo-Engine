#pragma once
#include "common.h"
#include "GPUBuffer.h"

namespace Core {

	struct GPUMemPage : public Resource{
		void* CPUPtr;
		UINT Size;

		GPUMemPage(ID3D12Resource* res,D3D12_RESOURCE_STATES CurrentState,UINT Size):
		Resource(res,CurrentState),CPUPtr(nullptr),Size(Size){
			mResource->Map(0, nullptr, &CPUPtr);
		}

		~GPUMemPage() { mResource->Unmap(0, nullptr); Release(); }
	};

	struct GPUMemAlloc {
		void* CPUPtr = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
		Resource& mResource;
		UINT Size;
		UINT Offset;
		
		GPUMemAlloc(GPUMemPage& res,UINT Size,UINT Offset):
		mResource(res),GPUAddress(res.GetVirtualAddress() + Offset),
		Size(Size),Offset(Offset)
		{ 
			CPUPtr = reinterpret_cast<uint8_t*>(res.CPUPtr) + Offset;
		}
	};

	class UploadMemAllocator {
	public:
		UploadMemAllocator():
		mCurrentPage(CreateNewResource()),
		mCurrentOffset(0){
			MemPool.push_back(std::unique_ptr<GPUMemPage>(mCurrentPage));
		}

		GPUMemAlloc Allocate(UINT size);

		void Release(FenceVal fence);

		void ReleaseAll();

		~UploadMemAllocator() { ReleaseAll(); }
	private:
		void FlushUsedMem(FenceVal fence);

		GPUMemPage* CreateNewResource(UINT Size = mResourceSize);

		std::vector<std::unique_ptr<GPUMemPage>> MemPool;
		std::queue<std::pair<FenceVal, GPUMemPage*>> UsedLargePages;
		std::queue < std::pair <FenceVal, GPUMemPage*>> UsedMem;
		std::queue<GPUMemPage*> AvaliableMem;
		std::queue<std::pair<bool, GPUMemPage*>> ToReleasePages;

		GPUMemPage* mCurrentPage;
		UINT mCurrentOffset;

		static const UINT mResourceSize;
	};

	class BufferManager;
}