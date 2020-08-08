#include "D3DDescriptorHandleAllocator.h"
#include "Common.h"
using namespace Game;

const D3DDescriptorHandle D3DDescriptorHandle::invalid = {
	0,0
};

bool D3DDescriptorHandleAllocator::initiailize(ID3D12Device* device) {
	this->device = device;


	mSRVUAVHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCurrSRVUAVHeap = nullptr;
	mCurrSRVUAVHeapSize = 1024, mCurrSRVUAVHeapOffset = 0;
	mCurrSRVUAVHeap = createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,mCurrSRVUAVHeapSize);

	if (mCurrSRVUAVHeap == nullptr) {
		return false;
	}
	return true;
}

ComPtr<ID3D12DescriptorHeap> D3DDescriptorHandleAllocator::createDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE type, size_t size) {

	D3D12_DESCRIPTOR_HEAP_DESC hDesc;
	hDesc.Type = type;
	hDesc.NodeMask = 0;
	hDesc.NumDescriptors = size;
	hDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ID3D12DescriptorHeap* heap;
	HRESULT hr = device->CreateDescriptorHeap(&hDesc, IID_PPV_ARGS(&heap));
	
	if (FAILED(hr)) {
		return nullptr;
	}
	return heap;
}

D3DDescriptorHandle D3DDescriptorHandleAllocator::getDescriptorHandle(
						UUID uuid,D3D12_DESCRIPTOR_HEAP_TYPE type) {
	auto find = handles.find(uuid);
	if (find == handles.end()) {
		D3DDescriptorHandle handle = D3DDescriptorHandle::invalid;
		return handle;
	}

	return find->second;
}

D3DDescriptorHandle D3DDescriptorHandleAllocator::AllocateNewHandle(
						UUID uuid,D3D12_DESCRIPTOR_HEAP_TYPE type) {
	if (handles.find(uuid) != handles.end()) {
		Log("d3d12 : the resource %s have already uploaded to the descriptor heap \n", uuid.to_string().c_str());
		return D3DDescriptorHandle::invalid;
	}

	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		if (mCurrSRVUAVHeapOffset + 1 > mCurrSRVUAVHeapSize) {
			mCurrSRVUAVHeap = createDescriptorHeap(type, mCurrSRVUAVHeapSize * 2);
			handles.clear();
			mCurrSRVUAVHeapOffset = 0;
			mCurrSRVUAVHeapSize = mCurrSRVUAVHeapSize * 2;
		}
		D3DDescriptorHandle handle;
		handle.cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			mCurrSRVUAVHeap->GetCPUDescriptorHandleForHeapStart()).Offset(mCurrSRVUAVHeapOffset, mSRVUAVHandleSize);
		handle.gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			mCurrSRVUAVHeap->GetGPUDescriptorHandleForHeapStart()
		).Offset(mCurrSRVUAVHeapOffset, mSRVUAVHandleSize);

		handles[uuid] = handle;
		mCurrSRVUAVHeapOffset++;

		return handle;
	}
	else {
		Log("d3d12 : invalid descriptor heap type %d , fail to load resource %s to descriptor heap",type,
					uuid.to_string().c_str());
		return D3DDescriptorHandle::invalid;
	}
}