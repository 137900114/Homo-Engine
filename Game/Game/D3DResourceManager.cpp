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


	UUID D3DResourceManager::uploadStaticBuffer(void* data, size_t size,
		D3D12_RESOURCE_STATES initState,bool isConstantBuffer) {

		size_t requiredWidth = size;
		if (isConstantBuffer) {
			//round up the buffer because constant buffer has to be the multiple of 256
			requiredWidth = (requiredWidth + 255) & ~(255);
		}

		StaticResource res;
		res.uploadBuffer = gpuAllocator.CreateResource(CD3DX12_RESOURCE_DESC::Buffer(requiredWidth), D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,nullptr);
		res.buffer = gpuAllocator.CreateResource(CD3DX12_RESOURCE_DESC::Buffer(requiredWidth), D3D12_HEAP_TYPE_DEFAULT,
			initState, nullptr);
		res.type = BUFFER;

		if (res.buffer == nullptr || res.uploadBuffer == nullptr) {
			gpuAllocator.ReleaseResource(res.buffer);
			gpuAllocator.ReleaseResource(res.uploadBuffer);
			return UUID::invalid;
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
			gpuAllocator.ReleaseResource(res.buffer);
			return UUID::invalid;
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
		//we throw a invalid buffer size state code
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
		if (token == UUID::invalid)
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
				if(item.second.type == BUFFER)
					item.second.uploadBuffer->Unmap(0,nullptr);
				gpuAllocator.ReleaseResource(item.second.uploadBuffer);
			}
		}


		if (barriers.empty()) return;

		cmdLis->ResourceBarrier(barriers.size(), barriers.data());

		//distribute copy commands
		for (auto item : uploadWaitingList) {
			if(item->type == BUFFER)
				cmdLis->CopyResource(item->buffer.Get(),item->uploadBuffer.Get());
			else if(item->type == TEXTURE2D){
				D3D12_SUBRESOURCE_DATA data;
				data.pData = item->textureData.data;
				data.RowPitch = item->textureData.rowPitch;
				data.SlicePitch = item->textureData.slicePitch;

				UpdateSubresources(cmdLis,
					item->buffer.Get(),
					item->uploadBuffer.Get(),
					0, 0, 1,
					&data);
			}
			else if (item->type == TEXTURE2D_ARRAY) {
				D3D12_SUBRESOURCE_DATA* subData = reinterpret_cast<D3D12_SUBRESOURCE_DATA*>(item->textureArray.subTex);
				UpdateSubresources(cmdLis,
					item->buffer.Get(),
					item->uploadBuffer.Get(),
					0, 0, item->textureArray.subTexNum,
					subData);
			}
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
			Log("d3d 12 : resource manager : invalid target resource to copy\n");
			return false;
		}

		if (_target->second.currState != D3D12_RESOURCE_STATE_COPY_DEST) {
			Log("d3d 12 : resource manager : invalid target resource state, the resource state must be "
				"copy dest while copying\n");
			return false;
		}

		if (_ssource != staticResource.end()) {
			if (!_ssource->second.uploaded || _ssource->second.currState != D3D12_RESOURCE_STATE_COPY_SOURCE) {
				Log("d3d 12 : resource manager : invalid state for copy source it is not uploaded or the"
					"resource state is not D3D12_RESOURCE_STATE_COPY_DEST currently\n");
				return false;
			}
			cmdLis->CopyResource(_target->second.buffer.Get(),
				_ssource->second.buffer.Get());
			
		}
		else{
			auto _dsource = dynamicResource.find(target);
			if (_dsource != dynamicResource.end()) {
				Log("d3d 12 : resource manager : invalid source resource for copy\n");
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

	UUID D3DResourceManager::uploadTexture(void* data, size_t width, size_t height, D3D12_RESOURCE_STATES initState,
		size_t rowPitch, size_t mipLevel,DXGI_FORMAT format) {
		mipLevel = 1;
		//currently the mipLevel can only be 1
		
		D3D12_RESOURCE_DESC tDesc = {};
		tDesc.Format = format;
		tDesc.DepthOrArraySize = 1;
		tDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		tDesc.Width = width;
		tDesc.Height = height;
		tDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tDesc.MipLevels = mipLevel;
		tDesc.SampleDesc = {1,0};
		
		//we will create upload buffer later;
		StaticResource sRes;
		sRes.buffer = gpuAllocator.CreateResource(tDesc, D3D12_HEAP_TYPE_DEFAULT,initState);
		sRes.uploaded = false;
		sRes.uploadBuffer = nullptr;
		sRes.textureData.data = data;
		sRes.textureData.rowPitch = rowPitch;
		sRes.textureData.slicePitch = rowPitch * height;
		sRes.currState = initState;
		sRes.type = TEXTURE2D;

		if (sRes.buffer == nullptr) return UUID::invalid;

		size_t uploadRequiredSize = GetRequiredIntermediateSize(sRes.buffer.Get(),0,1);
		sRes.uploadBuffer = gpuAllocator.CreateResource(CD3DX12_RESOURCE_DESC::Buffer(uploadRequiredSize),
			D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

		if (sRes.uploadBuffer == nullptr) {
			gpuAllocator.ReleaseResource(sRes.uploadBuffer);
			gpuAllocator.ReleaseResource(sRes.buffer);
			return UUID::invalid;
		}

		UUID newID = UUID::generate();
		while (staticResource.find(newID) != staticResource.end() ||
			dynamicResource.find(newID) != dynamicResource.end()) {
			newID = UUID::generate();
		}

		staticResource[newID] = sRes;
		return newID;
	}
}

Game::UUID Game::D3DResourceManager::uploadTexture(Texture& image, D3D12_RESOURCE_STATES initState, size_t mipLevel) {
	switch (image.type) {
	case TEXTURE_TYPE::CUBE:
		return uploadCubeTexture(image.width,image.height,
			initState,image.subDataDescriptor);
	case TEXTURE_TYPE::TEXTURE2D:
		return uploadTexture(image.data.data, image.width, image.height, initState, image.rowPitch,
			mipLevel, DXGI_FORMAT_R8G8B8A8_UNORM);
	default:
		Log("d3d12 resource manager : fail to upload texture,invalid texture type");
		return Game::UUID::invalid;
	}
}


Game::UUID Game::D3DResourceManager::uploadCubeTexture(size_t width,size_t height,D3D12_RESOURCE_STATES initState,
		Game::SubTextureDescriptor* desc,DXGI_FORMAT format) {
	
	
	D3D12_RESOURCE_DESC rDesc = {};
	rDesc.Format = format;
	rDesc.DepthOrArraySize = 6;
	rDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	rDesc.Width = width, rDesc.Height = height;
	rDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rDesc.MipLevels = 1;
	rDesc.SampleDesc = {1,0};


	StaticResource sRes;
	sRes.textureArray.subTex = desc;
	sRes.textureArray.subTexNum = 6;
	sRes.type = TEXTURE2D_ARRAY;
	sRes.buffer = gpuAllocator.CreateResource(rDesc, D3D12_HEAP_TYPE_DEFAULT, initState);
	sRes.currState = initState;
	sRes.uploadBuffer = nullptr;
	sRes.uploaded = false;

	if (sRes.buffer == nullptr) {
		return Game::UUID::invalid;
	}

	size_t requiredSize = GetRequiredIntermediateSize(sRes.buffer.Get(), 0, 6);
	sRes.uploadBuffer = gpuAllocator.CreateResource( CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	if (sRes.uploadBuffer == nullptr) {
		gpuAllocator.ReleaseResource(sRes.buffer);
		return UUID::invalid;
	}

	Game::UUID uuid = Game::UUID::generate();
	while (staticResource.find(uuid) != staticResource.end()) {
		uuid = Game::UUID::generate();
	}
	
	staticResource[uuid] = sRes;
	return uuid;
}