#include "D3DResourceManager.h"
#include "d3d12x.h"
#include "Common.h"

namespace Game {

	//gpu resource allocator based on d3d11 style api CreateCommitedResource
	class CommitAllocator {
	public:
		void initialize(ID3D12Device* device);

		ComPtr<ID3D12Resource> CreateResource(D3D12_RESOURCE_DESC desc,D3D12_HEAP_TYPE type,
			D3D12_RESOURCE_STATES initState,const D3D12_CLEAR_VALUE* value = nullptr);

		void ReleaseResource(ComPtr<ID3D12Resource>& ptr);

		void finalize() {}
	private:
		ID3D12Device* device;
	};

	/*
	a morden gpu resource allocator based on memory heap is recommended
	class HeapAllocator {

	};
	*/

	CommitAllocator gpuAllocator;


	void CommitAllocator::initialize(ID3D12Device* device) {
		this->device = device;
	}

	ComPtr<ID3D12Resource> CommitAllocator::CreateResource(D3D12_RESOURCE_DESC desc,
		D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES initState,
		const D3D12_CLEAR_VALUE* value) {
		ComPtr<ID3D12Resource> target;

		HRESULT hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(type),
			D3D12_HEAP_FLAG_NONE,
			&desc,initState,
			value,IID_PPV_ARGS(&target)
		);

		if (FAILED(hr)) {
			return nullptr;
		}

		return target;
	}

	
	void CommitAllocator::ReleaseResource(ComPtr<ID3D12Resource>& ptr) {
		ptr = nullptr;
	}

	void D3DResourceManager::initialize(ID3D12Device* dev) {
		this->mDevice = dev;
		gpuAllocator.initialize(dev);
	}


	UUID D3DResourceManager::uploadStatic(void* data, size_t size, D3D12_RESOURCE_DESC desc,
		D3D12_RESOURCE_STATES initState,bool isConstantBuffer) {
		if (isConstantBuffer) {
			//round up the buffer because constant buffer has to be the multiple of 256
			desc.Width = (desc.Width + 255) & ~(255);
		}

		StaticResource res;
		res.uploadBuffer = gpuAllocator.CreateResource(desc, D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,nullptr);
		res.buffer = gpuAllocator.CreateResource(desc, D3D12_HEAP_TYPE_DEFAULT,
			initState, nullptr);

		if (res.buffer == nullptr || res.uploadBuffer == nullptr) {
			return UUID::invaild;
		}

		res.uploadBuffer->Map(0, nullptr,&res.bufferWriter);
		memcpy(res.bufferWriter, data, size);

		res.uploaded = false;
		res.currState = initState;

		UUID newID = Game::UUID::generate();
		while (staticResource.find(newID) != staticResource.end()
			|| dynamicResource.find(newID) != dynamicResource.end())  newID = Game::UUID::generate();

		staticResource[newID] = res;

		return newID;
	}

	UUID D3DResourceManager::uploadDynamic(void* data, size_t size,D3D12_RESOURCE_DESC desc,bool isConstantBuffer) {
		if (isConstantBuffer) {
			//round up the buffer because constant buffer has to be the multiple of 256
			desc.Width = (desc.Width + 255) & ~(255);
			size = desc.Width;
		}

		DynamicResource res;
		res.buffer = gpuAllocator.CreateResource(desc, D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

		if (res.buffer == nullptr) {
			return UUID::invaild;
		}

		res.buffer->Map(0, nullptr,& res.bufferWriter);
		res.size = desc.Width;

		memcpy(res.bufferWriter, data, size);
		
		UUID newID = Game::UUID::generate(); 
		while (staticResource.find(newID) != staticResource.end()
		|| dynamicResource.find(newID) != dynamicResource.end())  newID = Game::UUID::generate();


		dynamicResource[newID] = res;

		return newID;
	}

	ID3D12Resource* D3DResourceManager::getResource(UUID token) {
		auto siter = staticResource.find(token);
		if (siter != staticResource.end()) {
			if(siter->second.uploaded)
				return siter->second.buffer.Get();
			else {
				Log("d3d 12 resource loader : stall for 1 frame for loading\n");
				return nullptr;
			}
		}

		auto diter = dynamicResource.find(token);
		if (diter != dynamicResource.end()) {
			return diter->second.buffer.Get();
		}

		return nullptr;
	}


	D3DResourceManager::WRITE_STATE D3DResourceManager::write(UUID token, void* data, size_t size) {
		auto diter = dynamicResource.find(token);
		if (diter == dynamicResource.end()) {
			return NOT_FOUND;
		}
		//if the original buffer capacity is smaller than the buffer size
		//we throw a invaild buffer size state code
		else if (diter->second.size < size) {
			return INVAILD_BUFFER_SIZE;
		}
		else {
			memcpy(diter->second.bufferWriter, data, size);
			return SUCCESS;
		}
	}

	bool D3DResourceManager::releaseResource(UUID token) {
		//release a empty token is vaild.we do nothing and return a true
		if (token == UUID::invaild)
			return true;

		auto siter = staticResource.find(token);
		if (siter != staticResource.end()) {
			gpuAllocator.ReleaseResource(siter->second.buffer);
			if (siter->second.bufferWriter != nullptr) {
				siter->second.uploadBuffer->Unmap(0, nullptr);
			}
			gpuAllocator.ReleaseResource(siter->second.uploadBuffer);
			staticResource.erase(siter);
			return true;
		}

		auto diter = dynamicResource.find(token);
		if (diter != dynamicResource.end()) {
			diter->second.buffer->Unmap(0,nullptr);
			gpuAllocator.ReleaseResource(diter->second.buffer);
			dynamicResource.erase(diter);
			return true;
		}

		return false;
	}

	void D3DResourceManager::tick(ID3D12GraphicsCommandList* cmdLis) {
		for (auto& item : staticResource) {
			if (!item.second.uploaded) {
				uploadWaitingList.push_back(&item.second);
				if (item.second.currState != D3D12_RESOURCE_STATE_COPY_DEST)
					barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
						item.second.buffer.Get(),item.second.currState,
						D3D12_RESOURCE_STATE_COPY_DEST));
			}
			else if (item.second.uploadBuffer != nullptr) {
				//if the data have been uploaded but the upload buffer is not released 
				//release the buffer
				item.second.uploadBuffer->Unmap(0,nullptr);
				gpuAllocator.ReleaseResource(item.second.uploadBuffer);
			}
		}

		if (barriers.empty()) return;

		cmdLis->ResourceBarrier(barriers.size(), barriers.data());

		//distribute copy commands
		for (auto item : uploadWaitingList) {
			cmdLis->CopyResource(item->buffer.Get(),item->uploadBuffer.Get());
		}

		barriers.clear();

		for (auto& item : uploadWaitingList) {
			item->uploaded = true;
			if (item->currState != D3D12_RESOURCE_STATE_COPY_DEST) {
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
					item->buffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST,
					item->currState
				));
			}
		}

		cmdLis->ResourceBarrier(barriers.size(), barriers.data());

		barriers.clear();
		uploadWaitingList.clear();
	}


	bool D3DResourceManager::immediateStateTransition(UUID token,D3D12_RESOURCE_STATES targetState,
		ID3D12GraphicsCommandList* cmdLis) {
		auto siter = staticResource.find(token);
		
		if (siter == staticResource.end()) {
			return false;
		}

		cmdLis->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				siter->second.buffer.Get(),siter->second.currState,
				targetState)
			);

		siter->second.currState = targetState;

		return true;
	}

	bool D3DResourceManager::copyData(UUID target,UUID source, ID3D12GraphicsCommandList* cmdLis) {
		auto _target = staticResource.find(target),
			_ssource = staticResource.find(source);


		if (_target == staticResource.end()) {
			Log("d3d 12 : resource manager : invaild target resource to copy\n");
			return false;
		}

		if (_target->second.currState != D3D12_RESOURCE_STATE_COPY_DEST) {
			Log("d3d 12 : resource manager : invaild target resource state, the resource state must be "
				"copy dest while copying\n");
			return false;
		}

		if (_ssource != staticResource.end()) {
			if (!_ssource->second.uploaded || _ssource->second.currState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
				Log("d3d 12 : resource manager : invaild state for copy source it is not uploaded or the"
					"resource state is not D3D12_RESOURCE_STATE_COPY_DEST currently\n");
				return false;
			}
			cmdLis->CopyResource(_target->second.buffer.Get(),
				_ssource->second.buffer.Get());
			
		}
		else{
			auto _dsource = dynamicResource.find(target);
			if (_dsource != dynamicResource.end()) {
				Log("d3d 12 : resource manager : invaild source resource for copy\n");
				return false;
			}

			cmdLis->CopyResource(_target->second.buffer.Get(),
				_dsource->second.buffer.Get());
		}

		return true;
	}

	void D3DResourceManager::finalize() {
		for (auto& item : staticResource) {
			gpuAllocator.ReleaseResource(item.second.buffer);
			if (item.second.uploadBuffer != nullptr) {
				item.second.uploadBuffer->Unmap(0,nullptr);
				gpuAllocator.ReleaseResource(item.second.uploadBuffer);
			}
		}

		for (auto& item : dynamicResource) {
			item.second.buffer->Unmap(0,nullptr);
			gpuAllocator.ReleaseResource(item.second.buffer);
		}

		gpuAllocator.finalize();
	}
}