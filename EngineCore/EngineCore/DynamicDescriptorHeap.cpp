#include "DynamicDescriptorHeap.h"
#include "Device.h"
#include "trivial.h"

using namespace std;
using namespace Core;

std::unique_ptr<DescriptorHeapAllocator> DescriptorHeapAllocator::allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
	nullptr,nullptr,nullptr,nullptr
};

ID3D12DescriptorHeap* DescriptorHeapAllocator::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_FLAGS flag) {
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.NumDescriptors = DESCRIPTOR_HEAP_SIZE;
	desc.Type = m_Type;
	desc.NodeMask = 1;
	desc.Flags = flag;

	ID3D12DescriptorHeap* res = nullptr;
	ASSERT_HR(Device::GetCurrentDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&res)),
		"DescriptorHeapAllocator : Failed to create new descriptor heap");
	return res;
}

ID3D12DescriptorHeap* DescriptorHeapAllocator::AllocateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_FLAGS flag) {
	if (!m_AvaliableDescriptorHeaps[flag].empty()) {
		ID3D12DescriptorHeap* heap = m_AvaliableDescriptorHeaps[flag].front();
		m_AvaliableDescriptorHeaps[flag].pop();
		return heap;
	}

	ComPtr<ID3D12DescriptorHeap> newHeap = CreateDescriptorHeap(flag);
	m_DescriptorHeaps.push_back(newHeap);
	return newHeap.Get();
}

void DescriptorHeapAllocator::ReleaseUsedHeaps(
	FenceVal FenceV, std::vector<ID3D12DescriptorHeap*>& toRelease,D3D12_DESCRIPTOR_HEAP_FLAGS flag) {

	while (!m_UsedDescriptorHeaps[flag].empty() && m_UsedDescriptorHeaps[flag].front().first < FenceV) {
		ID3D12DescriptorHeap* newHeap = m_UsedDescriptorHeaps[flag].front().second;
		m_UsedDescriptorHeaps[flag].pop();
		m_AvaliableDescriptorHeaps[flag].push(newHeap);
	}

	for (auto heap : toRelease)
		m_UsedDescriptorHeaps[flag].push(make_pair(FenceV, heap));

	toRelease.clear();
}

D3D12_CPU_DESCRIPTOR_HANDLE TemporaryDescriptorHeap::AllocateDescriptorHandle(size_t Num) {
	ASSERT_WARNING(Num > DESCRIPTOR_HEAP_SIZE,"TemporaryDescriptorHeap::AllocateDescriptorHandle\
		the number of handles to allocate is out of the Maximun size(1024)");

	if (!m_CurrentHeap || Num + m_CurrentOffset > DESCRIPTOR_HEAP_SIZE) {
		if(!m_CurrentHeap)
			m_RetieredDescriptorHeaps.push_back(m_CurrentHeap);
		m_CurrentHeap = DescriptorHeapAllocator::AllocateDescriptorHeap(m_Type,D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		m_CurrentOffset = 0;
		m_HeapStartHandle = DescriptorHandle(m_CurrentHeap->GetCPUDescriptorHandleForHeapStart());
	}

	D3D12_CPU_DESCRIPTOR_HANDLE res = m_HeapStartHandle.Offset(m_DescriptorOffset, m_CurrentOffset).cpuHandle;
	m_CurrentOffset += Num;

	return res;
}

void StaticDescriptorHeap::CreateDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC desc;

	desc.NumDescriptors = m_HeapSize;
	desc.NodeMask = m_NodeMask;
	desc.Flags = m_Flag;
	desc.Type = m_Type;

	ASSERT_HR(
		Device::GetCurrentDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)),
		"StaticDescriptorHeap::CreateDescriptorHeap Failed to create descriptor heap");

	m_CurrentOffset = 0;
	m_DescriptorHandleStart = DescriptorHandle(m_Heap->GetCPUDescriptorHandleForHeapStart(),
		m_Heap->GetGPUDescriptorHandleForHeapStart());
}

DescriptorHandle StaticDescriptorHeap::Allocate(UINT count) {
	ASSERT_WARNING(count + m_CurrentOffset > m_HeapSize,"StaticHeap::Allocate there are too many descriptors.");

	DescriptorHandle handle = m_DescriptorHandleStart.Offset(m_DescriptorOffsetSize, m_CurrentOffset);
	m_CurrentOffset += count;
	return handle;
}

const UINT DynamicDescriptorHeap::m_DescriptorHeapSize = DESCRIPTOR_HEAP_SIZE;

UINT DynamicDescriptorHeap::ComputeCurrentNeededSize() {
	UINT index = 0;
	uint32_t bitMap = m_DescriptorTableBitMap;
	UINT size = 0;

	while (_BitScanForward((DWORD*)&index,bitMap)) {
		bitMap ^= 1 << index;
		size += m_DescriptorTableSize[index];
	}

	return size;
}

void DynamicDescriptorHeap::ReserveSpaceForRootSignature(RootSignature* rootSig) {
	m_DescriptorTableBitMap = rootSig->m_DescriptorTableBitMap;
	uint32_t bitMap = m_DescriptorTableBitMap;

	UINT index = 0;
	UINT CurrentOffset = 0;
	while (_BitScanForward((DWORD*)&index,bitMap)) {
		m_DescriptorTableSize[index] = rootSig->m_DescriptorTableSize[index];
		m_DescriptorTableOffsets[index] = CurrentOffset + m_HandleCache;
		CurrentOffset += m_DescriptorTableSize[index];

		bitMap ^= 1 << index;
	}
}

void DynamicDescriptorHeap::BindDescriptorOnHeap(UINT rootIndex,UINT num, UINT offset,
	D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {

	ASSERT_WARNING(!(m_DescriptorTableBitMap & (1 << rootIndex)),"DynamicDescriptorHeap::BindDescriptorOnHeap : \
    the root index to bind is not a descriptor table");
	ASSERT_WARNING(offset + num > m_DescriptorTableSize[rootIndex],"DynamicDescriptorHeap::BindDescriptorHeap : \
    out of the range of the descriptor table ");

	memcpy(m_DescriptorTableOffsets[rootIndex] + offset,handles,sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * num);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	if (!HasSpace(1))
		GetNewDescriptorHeap();

	DescriptorHandle currentHandle = m_HandleStart.Offset(m_DescriptorHeapIncreaseOffset,m_CurrentOffset);
	Device::GetCurrentDevice()->CopyDescriptorsSimple(1, currentHandle.cpuHandle,
		handle, m_Type);
	
	return currentHandle.gpuHandle;
}

void DynamicDescriptorHeap::GetNewDescriptorHeap() {
	if(m_Heap) 
		m_RetiredHeap.push_back(m_Heap);

	m_CurrentOffset = 0;
	m_Heap = DescriptorHeapAllocator::AllocateDescriptorHeap(m_Type,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	m_HandleStart = DescriptorHandle(
		m_Heap->GetCPUDescriptorHandleForHeapStart(),
		m_Heap->GetGPUDescriptorHandleForHeapStart()
	);
}

void DynamicDescriptorHeap::SetOnCommandList(
	ID3D12GraphicsCommandList* cmdLis,void (ID3D12GraphicsCommandList::* setFunc)(UINT,D3D12_GPU_DESCRIPTOR_HANDLE))
{
	UINT needSpace = ComputeCurrentNeededSize();
	if (!HasSpace(needSpace)) 
		GetNewDescriptorHeap();

	DescriptorHandle allocatedHandle = m_HandleStart.Offset(m_DescriptorHeapIncreaseOffset, m_CurrentOffset);
	m_CurrentOffset += needSpace;

	uint32_t bitMap = m_DescriptorTableBitMap;
	UINT SrcSize[16] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };

	UINT rootIndex;
	while (_BitScanForward((DWORD*)&rootIndex,bitMap)) {
		(cmdLis->*setFunc)(rootIndex,allocatedHandle.gpuHandle);

		D3D12_CPU_DESCRIPTOR_HANDLE* handleStart = m_DescriptorTableOffsets[rootIndex];
		UINT size = m_DescriptorTableSize[rootIndex];

		Device::GetCurrentDevice()->CopyDescriptors(1,
			&allocatedHandle.cpuHandle,&size,size,
			handleStart,SrcSize,m_Type);

		allocatedHandle.Offset(m_DescriptorHeapIncreaseOffset,size);

		bitMap ^= 1 << rootIndex;
	}

}