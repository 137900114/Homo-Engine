#pragma once
#include "common.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include  "GPUMemory.h"
#include  "DynamicDescriptorHeap.h"

namespace Core {

	class CommandAllocatorPool {
	public:
		ID3D12CommandAllocator* Allocate(FenceVal fence);

		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) : Type(Type) {}

		void Release(UINT fence,ID3D12CommandAllocator* target);

		void ReleaseAll();
	private:

		std::vector<ComPtr<ID3D12CommandAllocator>>          mAllocatorPool;
		std::queue<std::pair<UINT, ID3D12CommandAllocator*>> mUsedAllocator;
		
		D3D12_COMMAND_LIST_TYPE Type;
	};

	class CommandQueue {
	public:
		inline ID3D12CommandAllocator* RequestAllocator(FenceVal fence) {
			return mAllocatorPool.Allocate(fence);
		}
		inline void ReleaseAllocator(FenceVal fence, ID3D12CommandAllocator* alloc) {
			mAllocatorPool.Release(fence, alloc);
		}

		CommandQueue(D3D12_COMMAND_LIST_TYPE Type):
			mFence(nullptr),
			mAllocatorPool(Type),
			mEventHandle(NULL),
			mNextFenceValue((FenceVal)Type << 60),
			mQueue(nullptr),
			mType(Type)
		{}

		void Initialize();

		~CommandQueue() { 
			Close();
		}

		void Close();

		inline ID3D12CommandQueue* GetCommandQueue() { return  mQueue.Get(); }

		FenceVal ExcuteCommandList(ID3D12GraphicsCommandList* cmdLis);

		inline FenceVal GetNextFenceValue() { return mNextFenceValue; }
		inline FenceVal GetCompletedFenceVal() { return mFence->GetCompletedValue(); }

		bool IsFenceCompeleted(FenceVal fence);
		FenceVal IncreaseAndSignalFenceValue();

		void WaitForFence(FenceVal FenceVal);

		inline  void WaitForIdle() { WaitForFence(IncreaseAndSignalFenceValue()); }

	private:
		ComPtr<ID3D12Fence> mFence;
		CommandAllocatorPool mAllocatorPool;
		ComPtr<ID3D12CommandQueue> mQueue;
		HANDLE mEventHandle;

		FenceVal mNextFenceValue;
		D3D12_COMMAND_LIST_TYPE mType;

	};
		
	class CommandQueueManager {
	private:

		CommandQueue mGraphicQueue;
		CommandQueue mComputeQueue;
	public:
		void Initialize() {
			mGraphicQueue.Initialize();
			mComputeQueue.Initialize();
		}
		
		CommandQueueManager():
		mGraphicQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
		mComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{}

		void WaitForFence(FenceVal fence);
		bool IsFenceCompleted(FenceVal fence);
		void WaitForIdle() {
			mGraphicQueue.WaitForIdle();
			mComputeQueue.WaitForIdle();
		}
		void WaitForIdle(D3D12_COMMAND_LIST_TYPE Type);

		ID3D12GraphicsCommandList* CreateCommandList(D3D12_COMMAND_LIST_TYPE Type,ID3D12CommandAllocator** Allocator = nullptr);

		CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type);

		CommandQueue& GetGraphicQueue() { return mGraphicQueue; }
		CommandQueue& GetComputeQueue() { return mComputeQueue; }

		inline  FenceVal ExcuteByGraphicQueue(ID3D12GraphicsCommandList* cmdLis) { return mGraphicQueue.ExcuteCommandList(cmdLis); }
		inline  FenceVal ExcuteByComputeQueue(ID3D12GraphicsCommandList* cmdLis) { return mComputeQueue.ExcuteCommandList(cmdLis); }

		void ReleaseCommandAllocator(ID3D12CommandAllocator* Allocator,FenceVal fence);

		void Close(){
			mGraphicQueue.Close();
			mComputeQueue.Close();
		}
	};

	
	class CommandBuffer;
	class GraphicCommandBuffer;

	//这里先默认CommandBuffer的type是DIRECT
	#define COMMAND_BUFFER_DEAULT_COMMAND_LIST_TYPE  D3D12_COMMAND_LIST_TYPE_DIRECT

	class CommandBufferManager {
	private:
		CommandQueueManager* mLisManager;
		std::vector<unique_ptr<CommandBuffer>> mCommandBufferPool;
		std::queue<CommandBuffer*> mAvaliableCommandBuffers;
		static CommandBufferManager manager;

		CommandBufferManager() {
			mLisManager = nullptr;
		}

	public:

		static void Initialize(CommandQueueManager* mLisManager) {
			manager.mLisManager = mLisManager;
		}

		static CommandBuffer& AllocateBuffer();

		static void  ReleaseBuffer(CommandBuffer* toRelease) {
			manager.mAvaliableCommandBuffers.push(toRelease);
		}
	};


	class CommandBuffer {
		friend CommandBufferManager;
	protected:
		UploadMemAllocator cpuAllocator;

		RootSignature* RootSig;
		GraphicPSO* mGPSO;
		ComputePSO* mCPSO;

		ID3D12GraphicsCommandList* cmdLis;
		ID3D12CommandAllocator* cmdAlloc;
		CommandQueueManager* manager;
		
		DynamicDescriptorHeap mCBVRSVUAVHeap;

		ID3D12DescriptorHeap* toBindHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		UINT mCurrentHeapNum;
		uint8_t heapBit;

		CommandBuffer(ID3D12GraphicsCommandList* cmdLis,
			ID3D12CommandAllocator* cmdAlloc,
			CommandQueueManager* manager) :
			mCBVRSVUAVHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
			cmdLis(cmdLis),
			cmdAlloc(cmdAlloc),
			manager(manager){

			RootSig = nullptr;
			mGPSO = nullptr;
			mCPSO = nullptr;
			mCurrentHeapNum = 1;
			heapBit |= 1 << D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			memset(toBindHeaps, 0, sizeof(toBindHeaps));
		}

		
		
	public:
		static void Upload2Buffer(void* data,UINT Size,GPUBuffer& buffer);

		~CommandBuffer() {
			if (cmdLis) cmdLis->Release();
		}

		static CommandBuffer& Start();
		FenceVal Flush(bool WaitForCommand);
		FenceVal End(bool  WairForCommand = true) {
			FenceVal fence = Flush(WairForCommand);
			CommandBufferManager::ReleaseBuffer(this);
			return fence;
		}

		void Reset();

		void SetDescriptorHeap(ID3D12DescriptorHeap* Heap,D3D12_DESCRIPTOR_HEAP_TYPE mType);
		void BindDescriptorHeaps(bool Flush = false);

		void TransitionResource(Resource& target, D3D12_RESOURCE_STATES NewState);

		/*不推荐使用这个API*/
		void TransitionResource(ID3D12Resource* target, D3D12_RESOURCE_STATES start, D3D12_RESOURCE_STATES end) {
			cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				target, start, end));
		}

		void SetDescriptorTableHandle(UINT RootIndex,UINT Num,D3D12_CPU_DESCRIPTOR_HANDLE handles[],UINT Offset = 0);

		inline GraphicCommandBuffer& GetGraphicBuffer() {
			return *reinterpret_cast<GraphicCommandBuffer*>(this);
		}

		inline ID3D12GraphicsCommandList* GetCommandList() {
			return this->cmdLis;
		}
		/*
		inline ComputeCommandBuffer& GetComputeBuffer() {
			return *reinterpret_cast<ComputeCommandBuffer*>(this);
		}
		*/
		
	};


	class GraphicCommandBuffer : public CommandBuffer {
	public:
		void SetGraphicPipelineState(GraphicPSO* graphicPSO);

		void DrawIndexed(UINT indexCount, UINT startIndexLocation,UINT baseVertexLocation);
		void DrawIndexedInstance(UINT  indexCount,UINT startIndexLocation,
			UINT instanceCount,UINT startInstanceLocation,UINT baseVertexLocation);

		void SetConstantBufferView(UINT RootIndex,D3D12_GPU_VIRTUAL_ADDRESS buffer);
		void SetShaderResourceView(UINT RootIndex,D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle);
		
		inline void SetRenderTargetView(UINT RenderTargetNum,D3D12_CPU_DESCRIPTOR_HANDLE RTHandles[],
			D3D12_CPU_DESCRIPTOR_HANDLE DSHandle) {
			cmdLis->OMSetRenderTargets(RenderTargetNum, RTHandles,
				true, &DSHandle);
		}

		inline void SetDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE DSHandle) {
			cmdLis->OMSetRenderTargets(0, nullptr, true, &DSHandle);
		}

		inline void SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW vertBuffer) {
			cmdLis->IASetVertexBuffers(0, 1, &vertBuffer);
		}

		inline void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW indexBuffer) {
			cmdLis->IASetIndexBuffer(&indexBuffer);
		}

		inline void SetPrimitiveTopoloy(D3D12_PRIMITIVE_TOPOLOGY Type) {
			cmdLis->IASetPrimitiveTopology(Type);
		}

		void SetRootSignature(RootSignature* rootSig);
		void SetDynamicVertexBuffer(UINT element_size,UINT element_num,void* data);
		void SetDynamicIndexBuffer(UINT num,void* data);//默认index buffer的格式是UINT16

		void SetViewPortAndScissorRect(const D3D12_VIEWPORT& port,const D3D12_RECT& rect) {
			cmdLis->RSSetScissorRects(1, &rect);
			cmdLis->RSSetViewports(1, &port);
		}

		void ClearRenderTargetBuffer(D3D12_CPU_DESCRIPTOR_HANDLE rtv,float f[4]) {
			cmdLis->ClearRenderTargetView(rtv, f, 0, nullptr);
		}

		void ClearDepthAndStencilBuffer(D3D12_CPU_DESCRIPTOR_HANDLE dsv,float depth,float stencil) {
			cmdLis->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
		}

	
	};

	/*
	class ComputeCommandBuffer : public CommandBuffer {
	public:
		void SetComputePipelineState(ComputePSO* computePSO);

		void SetConstantBufferView(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS buffer);
		void SetShaderResourceView(UINT RootIndex, D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle);
		void SetUnorderedAccessView(UINT RootIndex, D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle);
	};*/

	
	
}