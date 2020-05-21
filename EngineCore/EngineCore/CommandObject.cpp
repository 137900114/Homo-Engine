#include "CommandObjects.h"
#include "Device.h"
#include "trivial.h"

using namespace Core;
using namespace std;

CommandBufferManager CommandBufferManager::manager;

ID3D12CommandAllocator* CommandAllocatorPool::Allocate(FenceVal fence) {

	if (!mUsedAllocator.empty() && fence >= mUsedAllocator.front().first) {
		ID3D12CommandAllocator* allocat = mUsedAllocator.front().second;
		mUsedAllocator.pop();
		return allocat;
	}

	ID3D12CommandAllocator* newAllocator;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommandAllocator(Type, IID_PPV_ARGS(&newAllocator));

	ASSERT_HR(hr, "CommandAllocatorPool::Allocate : Fail to create new allocator while allocating");

	mAllocatorPool.push_back(newAllocator);

	return newAllocator;
}

void CommandAllocatorPool::Release(UINT fence, ID3D12CommandAllocator* target) {
	mUsedAllocator.push(make_pair(fence, target));
}

void CommandAllocatorPool::ReleaseAll() {
	
	while (!mUsedAllocator.empty())
		mUsedAllocator.pop();

	for (int i = 0; i != mAllocatorPool.size(); i++)
		mAllocatorPool[i] = nullptr;

	mAllocatorPool.clear();
}

void CommandQueue::Initialize() {
	ID3D12Device* dev = Device::GetCurrentDevice();
	
	HRESULT hr = dev->CreateFence(mNextFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	ASSERT_HR(hr, "CommandQueue::Initialize : Fail to create fence");

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.NodeMask = 1;
	desc.Type = mType;

	hr = dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue));
	ASSERT_HR(hr, "CommandQueue::Initialize : Fail to create command queue");

	mEventHandle = CreateEventEx(nullptr, L"", NULL, EVENT_ALL_ACCESS);
	ASSERT_WARNING(!mEventHandle,"CommandQueue::Initialize : Fail to create Event")
}

FenceVal CommandQueue::IncreaseAndSignalFenceValue() {

	mNextFenceValue++;

	mQueue->Signal(mFence.Get(), mNextFenceValue);
	return mNextFenceValue;
}

bool CommandQueue::IsFenceCompeleted(FenceVal fence) {
	return mFence->GetCompletedValue() >= fence;
}

void CommandQueue::WaitForFence(FenceVal fence) {
	if (!IsFenceCompeleted(fence)) {

		mFence->SetEventOnCompletion(fence, mEventHandle);
		WaitForSingleObject(mEventHandle, INFINITE);
	}
}

//在执行之前必须先Close CommandList
FenceVal CommandQueue::ExcuteCommandList(ID3D12GraphicsCommandList* cmdLis) {
	
	ID3D12CommandList* toExcute[] = {cmdLis};
	mQueue->ExecuteCommandLists(_countof(toExcute), toExcute);

	return IncreaseAndSignalFenceValue();

}

void CommandQueue::Close() {

	mQueue = nullptr;
	mFence = nullptr;
	mAllocatorPool.ReleaseAll();
	if(mEventHandle)
		CloseHandle(mEventHandle);
}


CommandQueue& CommandQueueManager::GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type) {
	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return mGraphicQueue;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return mComputeQueue;
	default:
		DEBUG_OUTPUT("CommandQueueManager::GetCommandQueue : invaild command list type and by default we will get a graphic command queue as "
			"return value");
		__debugbreak();
		return mGraphicQueue;
	}
}

void CommandQueueManager::WaitForFence(FenceVal fence) {
	CommandQueue& target = GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(fence >> 60));
	target.WaitForFence(fence);
}

bool CommandQueueManager::IsFenceCompleted(FenceVal fence) {
	CommandQueue& target = GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(fence >> 60));
	return target.IsFenceCompeleted(fence);
}

void CommandQueueManager::WaitForIdle(D3D12_COMMAND_LIST_TYPE Type) {
	CommandQueue& target = GetCommandQueue(Type);
	return target.WaitForIdle();
}

ID3D12GraphicsCommandList* CommandQueueManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE Type,ID3D12CommandAllocator** allocator) {
	ID3D12Device* dev = Device::GetCurrentDevice();
	FenceVal fence;

	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_DIRECT :
		fence = mGraphicQueue.GetCompletedFenceVal();
		*allocator = mGraphicQueue.RequestAllocator(fence);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		fence = mComputeQueue.GetCompletedFenceVal();
		*allocator = mComputeQueue.RequestAllocator(fence);
		break;
	default:
		DEBUG_OUTPUT("CommandQueueManager::CreateCommandList : Invaild command list type");
		__debugbreak();
		return nullptr;
	}


	ID3D12GraphicsCommandList* cmdLis;
	HRESULT hr = dev->CreateCommandList(0, Type, *allocator, nullptr, IID_PPV_ARGS(&cmdLis));

	ASSERT_HR(hr, "CommandQueueManager::CreateCommandList : Fail to create command list");

	cmdLis->Close();
	return cmdLis;
}


void CommandQueueManager::ReleaseCommandAllocator(ID3D12CommandAllocator* Allocator, FenceVal fence) {
	CommandQueue& target = GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(fence >> 60));
	target.ReleaseAllocator(fence, Allocator);
}

CommandBuffer& CommandBufferManager::AllocateBuffer() {
	if (!manager.mAvaliableCommandBuffers.empty()) {
		CommandBuffer&  cmdBuffer = *manager.mAvaliableCommandBuffers.front();
		manager.mAvaliableCommandBuffers.pop();
		return  cmdBuffer;
	}

	ID3D12CommandAllocator* alloc;
	ID3D12GraphicsCommandList* cmdLis = manager.mLisManager->CreateCommandList(COMMAND_BUFFER_DEAULT_COMMAND_LIST_TYPE, &alloc);

	CommandBuffer* cmdBuffer = new CommandBuffer(cmdLis,alloc,manager.mLisManager);
	manager.mCommandBufferPool.push_back(std::unique_ptr<CommandBuffer>(cmdBuffer));
	return *cmdBuffer;
}

CommandBuffer& CommandBuffer::Start() {
	CommandBuffer& cmd = CommandBufferManager::AllocateBuffer();
	cmd.Reset();
	return cmd;
}

void GraphicCommandBuffer::SetRootSignature(RootSignature* rootSig) {
	ASSERT_WARNING(!rootSig->RootIsCreated(),"CommadBuffer::SetRootSignature : you should create rootsignature before set it");
	this->RootSig = rootSig;
	mCBVRSVUAVHeap.ReserveSpaceForRootSignature(rootSig);
	cmdLis->SetGraphicsRootSignature(rootSig->GetRootSignature());
}

void GraphicCommandBuffer::SetGraphicPipelineState(GraphicPSO* GPSO) {
	ASSERT_WARNING(!GPSO->GetPSO(), "CommandBuffer::SetGraphicPipelineState : you should create pso before set it");
	this->mGPSO = GPSO;
}

/*
void ComputeCommandBuffer::SetComputePipelineState(ComputePSO* CPSO) {
	ASSERT_WARNING(!CPSO->GetPSO(), "CommadBuffer::SetComputePipelineState : you should create pso before set it");
	this->mCPSO = CPSO;
}*/

void CommandBuffer::SetDescriptorHeap(ID3D12DescriptorHeap* Heap,D3D12_DESCRIPTOR_HEAP_TYPE Type) {
	ASSERT_WARNING((1 << Type) & heapBit,"CommadBuffer::SetDescriptorHeap :"
		" you shouldn't bind two heaps with the same type on a command buffer");
	toBindHeaps[mCurrentHeapNum++] = Heap;
}

void GraphicCommandBuffer::DrawIndexed(UINT indexCount,UINT startIndexLocation,UINT baseVertexLocation) {
	DrawIndexedInstance(indexCount, startIndexLocation, 1, 0,baseVertexLocation);
}

void CommandBuffer::BindDescriptorHeaps(bool Flush) {
	toBindHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = mCBVRSVUAVHeap.GetDescriptorHeap();
	cmdLis->SetDescriptorHeaps(mCurrentHeapNum, toBindHeaps);
	if (Flush) {
		memset(toBindHeaps, 0, sizeof(toBindHeaps));
		heapBit = 1 << D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		mCurrentHeapNum = 1;
	}
}

void GraphicCommandBuffer::DrawIndexedInstance(UINT indexCount,UINT startIndexLocation,
	UINT instanceNum,UINT startInstanceLocation,UINT baseVertexLocation) {
	ASSERT_WARNING(!mGPSO,"CommandBuffer::DrawIndexedInstance : you should set graphic pipeline state before draw");
	ASSERT_WARNING(!RootSig,"CommadBuffer::DrawIndexedInstance : you should set root signature before draw");
	
	cmdLis->SetPipelineState(mGPSO->GetPSO());
	mCBVRSVUAVHeap.SetOnGraphicCommandList(cmdLis);
	cmdLis->DrawIndexedInstanced(indexCount, instanceNum, startIndexLocation, startIndexLocation,baseVertexLocation);
}

void GraphicCommandBuffer::SetConstantBufferView(UINT RootIndex,D3D12_GPU_VIRTUAL_ADDRESS buffer) {
	cmdLis->SetGraphicsRootConstantBufferView(RootIndex, buffer);
}

void CommandBuffer::TransitionResource(Resource& target,D3D12_RESOURCE_STATES newState) {
	ASSERT_WARNING(target.mCurrentState == D3D12_RESOURCE_STATE_GENERIC_READ,
		"CommandBuffer::TransitionResource : Resource on upload heap can't be changed to other states!\n"
		"Seen on Documentation of Microsoft : However, resources created on UPLOAD heaps must start in and cannot change from the GENERIC_READ state"
		" since only the CPU will be doing writing.");

	if (target.mCurrentState != newState) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = target.Get();
		barrier.Transition.StateBefore = target.mCurrentState;
		barrier.Transition.StateAfter = newState;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		cmdLis->ResourceBarrier(1, &barrier);
	}

	target.mCurrentState = newState;
}

void CommandBuffer::Upload2Buffer(void* data,UINT Size,GPUBuffer& buffer) {
	CommandBuffer& cmd = CommandBuffer::Start();

	GPUMemAlloc alloc = cmd.cpuAllocator.Allocate(Size);
	memcpy(alloc.CPUPtr, data, Size);

	
	cmd.TransitionResource(buffer, D3D12_RESOURCE_STATE_COPY_DEST);
	cmd.cmdLis->CopyBufferRegion(buffer.Get(), 0,
		alloc.mResource.Get(), alloc.Offset, Size);
	cmd.TransitionResource(buffer, D3D12_RESOURCE_STATE_COMMON);

	cmd.End();
}

FenceVal CommandBuffer::Flush(bool WaitForCommand) {
	ASSERT_WARNING(!cmdAlloc,"CommandBuffer::Flush : You should set a command allocator before flush it");
	
	cmdLis->Close();
	FenceVal fence = manager->GetCommandQueue(COMMAND_BUFFER_DEAULT_COMMAND_LIST_TYPE).ExcuteCommandList(cmdLis);

	cpuAllocator.Release(fence);
	mCBVRSVUAVHeap.ReleaseRetiredHeap(fence);
	//cpuAllocator.FlushUsedMem(fence);

	if (WaitForCommand)
		manager->WaitForFence(fence);

	return fence;
}

void CommandBuffer::Reset() {
	mGPSO = nullptr;
	mCPSO = nullptr;
	RootSig = nullptr;

	cmdAlloc->Reset();
	cmdLis->Reset(cmdAlloc, nullptr);
}

void CommandBuffer::SetDescriptorTableHandle(UINT RootIndex, UINT Num, D3D12_CPU_DESCRIPTOR_HANDLE handles[],UINT Offset) {
	mCBVRSVUAVHeap.BindDescriptorOnHeap(RootIndex, Num,Offset,handles);
}

void GraphicCommandBuffer::SetShaderResourceView(UINT RootIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	mCBVRSVUAVHeap.BindDescriptorOnHeap(RootIndex, 1, 0, &handle);
}

void GraphicCommandBuffer::SetDynamicVertexBuffer(UINT element_size,UINT element_num,void* data) {
	UINT bufferSize = element_size * element_num;
	GPUMemAlloc mem = cpuAllocator.Allocate(bufferSize);

	memcpy(mem.CPUPtr, data, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = mem.GPUAddress;
	vbv.SizeInBytes = bufferSize;
	vbv.StrideInBytes = element_num;

	cmdLis->IASetVertexBuffers(0, 11, &vbv);
}

void GraphicCommandBuffer::SetDynamicIndexBuffer(UINT element_num,void* data) {
	UINT bufferSize = sizeof(uint16_t) * element_num;
	GPUMemAlloc mem = cpuAllocator.Allocate(bufferSize);

	memcpy(mem.CPUPtr, data, bufferSize);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = mem.GPUAddress;
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = bufferSize;

	cmdLis->IASetIndexBuffer(&ibv);
}