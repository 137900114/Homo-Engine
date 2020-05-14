#include "GPUMemory.h"
#include "Device.h"
#include "trivial.h"
using namespace Core;

const UINT UploadMemAllocator::mResourceSize = 0xfffff;


Resource* UploadMemAllocator::CreateNewResource(UINT Size) {
	D3D12_RESOURCE_DESC desc;

	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Alignment = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Height = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Width = Size;

	ID3D12Device* dev = Device::GetCurrentDevice();
	ID3D12Resource* newRes;
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&newRes)
	);

	ASSERT_HR(hr,"UploadMemAllocator::CreateNewResource : Fail to create new resource");

	Resource* result = new Resource(newRes,D3D12_RESOURCE_STATE_GENERIC_READ);
	return result;
}

GPUMemAlloc UploadMemAllocator::Allocate(UINT Size) {

	if (Size > mResourceSize) {
		Resource* LargePage = CreateNewResource(Size);
		GPUMemAlloc alloc(*LargePage, Size, 0);

		ToReleasePages.push(std::make_pair(true, LargePage));
		return alloc;
	}
	if (Size + mCurrentOffset > mResourceSize) {
		ToReleasePages.push(std::make_pair(false, mCurrentPage));
		mCurrentOffset = 0;
		if (AvaliableMem.empty()) {
			mCurrentPage = AvaliableMem.front();
			AvaliableMem.pop();
		}
		else {
			mCurrentPage = CreateNewResource();
			MemPool.push_back(std::unique_ptr<Resource>(mCurrentPage));
		}
	}

	GPUMemAlloc alloc(*mCurrentPage,Size,mCurrentOffset);
	mCurrentOffset += Size;

	return alloc;
}

void  UploadMemAllocator::Release(FenceVal fence) {
	while (!ToReleasePages.empty()) {
		auto pair = ToReleasePages.front();
		if (pair.first)
			UsedLargePages.push(std::make_pair(fence, pair.second));
		else
			UsedMem.push(std::make_pair(fence, pair.second));
		ToReleasePages.pop();
	}
	FlushUsedMem(fence);
}

void  UploadMemAllocator::FlushUsedMem(FenceVal fence) {
	while (!UsedLargePages.empty()) {
		if (UsedLargePages.front().first >= fence)
			break;
		Resource* res = UsedLargePages.front().second;
		UsedLargePages.pop();
		res->Release();
	}
	while (!UsedMem.empty()) {
		if (!UsedMem.front().first >= fence)
			break;
		Resource* res = UsedMem.front().second;
		UsedMem.pop();
		AvaliableMem.push(res);
	}
}

//queue不能直接clear掉实在太坑
#define CLEAR_QUEUE(q) while(!q.empty()) q.pop();

void UploadMemAllocator::ReleaseAll() {
	CLEAR_QUEUE(UsedLargePages);
	CLEAR_QUEUE(UsedMem);
	CLEAR_QUEUE(AvaliableMem);
	CLEAR_QUEUE(ToReleasePages);

	mCurrentPage = nullptr;
	mCurrentOffset = 0;

	for (auto& iter : MemPool) {
		iter->Release();
	}

	MemPool.clear();
}