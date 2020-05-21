#pragma once
#include "common.h"
#include "RootSignature.h"
#include "Device.h"

using namespace std;

namespace Core {

	enum {
		DESCRIPTOR_HEAP_SIZE = 1024
	};

	struct DescriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
			this->cpuHandle = cpuHandle;
			this->gpuHandle = gpuHandle;
		}

		DescriptorHandle() {
			cpuHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			gpuHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
		}

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
			gpuHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			this->cpuHandle = cpuHandle;
		}

		DescriptorHandle& operator=(const DescriptorHandle& handle) {
			cpuHandle = handle.cpuHandle;
			gpuHandle = handle.gpuHandle;
			return *this;
		}

		DescriptorHandle Offset(size_t DescriptorHandleSize, size_t HandleOffsetNum = 1) {
			DescriptorHandle result = *this;
			if (result.cpuHandle.ptr != GPU_VIRTUAL_ADDRESS_INVAILD)
				result.cpuHandle.ptr += DescriptorHandleSize* HandleOffsetNum;
			if (result.gpuHandle.ptr != GPU_VIRTUAL_ADDRESS_INVAILD)
				result.gpuHandle.ptr += DescriptorHandleSize * HandleOffsetNum;

			return result;
		}

		bool IsNull() { return cpuHandle.ptr == GPU_VIRTUAL_ADDRESS_INVAILD; }
		bool IsGPUVisible() { return gpuHandle.ptr != GPU_VIRTUAL_ADDRESS_INVAILD; }
	};

	//-------------------------需要修改------------------------------//

	class DescriptorHeapAllocator {
	public:
		DescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) :
			m_Type(Type) {}

		static std::unique_ptr<DescriptorHeapAllocator> allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		static ID3D12DescriptorHeap* AllocateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,D3D12_DESCRIPTOR_HEAP_FLAGS flag) {
			if (allocators[type] == nullptr)
				allocators[type] = make_unique<DescriptorHeapAllocator>(type);
			return allocators[type]->AllocateDescriptorHeap(flag);
		}

		static void ReleaseUsedHeaps(D3D12_DESCRIPTOR_HEAP_TYPE type, FenceVal Fence, std::vector<ID3D12DescriptorHeap*>& toRelease,
			D3D12_DESCRIPTOR_HEAP_FLAGS flag) {
			allocators[type]->ReleaseUsedHeaps(Fence, toRelease,flag);
		}

	private:
		ID3D12DescriptorHeap* CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_FLAGS flag);

		void ReleaseUsedHeaps(FenceVal Fence, std::vector<ID3D12DescriptorHeap*>& toRelease,D3D12_DESCRIPTOR_HEAP_FLAGS flag);

		ID3D12DescriptorHeap* AllocateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_FLAGS flag);

		std::vector<ComPtr<ID3D12DescriptorHeap>>			 m_DescriptorHeaps;
		std::queue<ID3D12DescriptorHeap*>					 m_AvaliableDescriptorHeaps[2];
		std::queue<std::pair<FenceVal, ID3D12DescriptorHeap*>> m_UsedDescriptorHeaps[2];
		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	};

	//-------需要在AllocateDescriptorHeap的时候需要给出对应的flag类型-------//


	//TemporaryDescriptorHeap is used to allocate descriptor that used for create views.
	//These descriptors are gpu invisible and can't be used to bind on render pipeline.
	class TemporaryDescriptorHeap {
	private:
		ID3D12DescriptorHeap* m_CurrentHeap;
		size_t m_CurrentOffset;
		std::vector<ID3D12DescriptorHeap*> m_RetieredDescriptorHeaps;
		DescriptorHandle m_HeapStartHandle;

		const D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		const UINT m_DescriptorOffset;

	public:

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptorHandle(size_t num = 1);

		void ReleaseAllHeaps(FenceVal Fence) {
			m_RetieredDescriptorHeaps.push_back(m_CurrentHeap);

			m_CurrentHeap = nullptr;
			m_CurrentOffset = 0;

			DescriptorHeapAllocator::ReleaseUsedHeaps(m_Type, Fence, m_RetieredDescriptorHeaps,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		}

		ID3D12DescriptorHeap* GetDescriptorHeap() { return m_CurrentHeap; }

		TemporaryDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type) :
			m_Type(Type),
			m_DescriptorOffset(Device::GetDescriptorIncreamentHandleOffset(Type))
		{
			m_CurrentHeap = nullptr;
			m_CurrentOffset = 0;
		}
	};

	//static heap主要用在体积比较小的heap上，static heap体积是确定的，不能改变，一些常驻资源可以提交到static heap上
	class StaticDescriptorHeap {
	private:
		const UINT m_HeapSize;
		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		D3D12_DESCRIPTOR_HEAP_FLAGS m_Flag;
		UINT m_NodeMask;

		//由于Heap不能从allocator上分配
		ComPtr<ID3D12DescriptorHeap> m_Heap;
		UINT m_CurrentOffset;
		UINT m_DescriptorOffsetSize;
		DescriptorHandle m_DescriptorHandleStart;

		void CreateDescriptorHeap();

	public:
		StaticDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE Type,UINT HeapSize,
			D3D12_DESCRIPTOR_HEAP_FLAGS Flag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			UINT NodeMask = 0):
		m_HeapSize(HeapSize),m_Type(Type),m_Flag(Flag),m_NodeMask(NodeMask){
			CreateDescriptorHeap();
			m_DescriptorOffsetSize = Device::GetDescriptorIncreamentHandleOffset(m_Type);
		}

		DescriptorHandle Allocate(UINT count = 1);
		
		ID3D12DescriptorHeap* GetDescriptorHeap() {
			return m_Heap.Get();
		}

		void Reset() {
			m_CurrentOffset = 0;
		}

		UINT GetCurrentOffset() { return m_CurrentOffset; }
	};


	//DirectX12-Graphic-Sample 的丐版，相对会浪费一些descriptor heap
	class DynamicDescriptorHeap {
	private:
		ID3D12DescriptorHeap* m_Heap;
		std::vector<ID3D12DescriptorHeap*> m_RetiredHeap;

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		UINT m_CurrentOffset;
		const UINT m_DescriptorHeapIncreaseOffset;
		static const UINT m_DescriptorHeapSize;//1024
		DescriptorHandle m_HandleStart;

		D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[512];
		UINT m_DescriptorTableSize[32];
		D3D12_CPU_DESCRIPTOR_HANDLE* m_DescriptorTableOffsets[32];
		uint32_t m_DescriptorTableBitMap;

		void SetOnCommandList(ID3D12GraphicsCommandList* commandLis,
			void (ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

		UINT ComputeCurrentNeededSize();

		void GetNewDescriptorHeap();

		bool HasSpace(UINT size) { return m_Heap != nullptr && (m_CurrentOffset + size <= m_DescriptorHeapSize); }

	public:

		DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type):
		m_DescriptorHeapIncreaseOffset(Device::GetDescriptorIncreamentHandleOffset(Type)){
			m_Type = Type;
			m_Heap = DescriptorHeapAllocator::AllocateDescriptorHeap(m_Type,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
			m_CurrentOffset = 0;
			m_DescriptorTableBitMap = 0;
			m_HandleStart = DescriptorHandle(
				m_Heap->GetCPUDescriptorHandleForHeapStart(),
				m_Heap->GetGPUDescriptorHandleForHeapStart()
			);
		}

		void ReserveSpaceForRootSignature(RootSignature* rootSig);

		void SetOnGraphicCommandList(ID3D12GraphicsCommandList* cmdLis) {
			SetOnCommandList(cmdLis, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
		}

		void SetOnComputeCommandList(ID3D12GraphicsCommandList* cmdLis) {
			SetOnCommandList(cmdLis,&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
		}

		void BindDescriptorOnHeap(UINT rootIndex,UINT num,UINT offset,D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

		D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle);

		ID3D12DescriptorHeap* GetDescriptorHeap() {
			return m_Heap;
		}

		void ReleaseRetiredHeap(FenceVal fence) {
			DescriptorHeapAllocator::ReleaseUsedHeaps(m_Type, fence, m_RetiredHeap,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		}
		
	};
}