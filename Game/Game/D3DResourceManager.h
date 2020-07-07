#pragma once

#include "d3dCommon.h"
#include "uuid.h"
#include <map>
#include "Buffer.h"


namespace Game {

	class D3DResourceManager {
		
		struct StaticResource{
			bool uploaded;
			ComPtr<ID3D12Resource> buffer;
			ComPtr<ID3D12Resource> uploadBuffer;
			void* bufferWriter;
			D3D12_RESOURCE_STATES currState;
		};

		//upload buffer's will always be D3D12_RESOURCE_STATE_GENERIC_READ
		struct DynamicResource {
			ComPtr<ID3D12Resource> buffer;
			size_t size;
			void* bufferWriter;
		};

	public:

		D3DResourceManager(ID3D12Device* device) {}

		//the resource uploaded staticly can't be changed,if the resource is a constant buffer we have to 
		//aliase it to 256 so we need to inform the resource manager by passing a true to parameter 'isConstantBuffer'
		//the function will return a invaild uuid when the operation fails
		UUID uploadStatic(Buffer& buf,D3D12_RESOURCE_DESC desc,D3D12_RESOURCE_STATES initState,bool isConstantBuffer = false);
		//the resources uploaded dynamicly can be changed by calling write. if the resource is a constant buffer we have to
		//aliase it to 256 so we need to inform the resource manager by passing a true to parameter 'isConstantBuffer'
		//the function will return a invaild uuid when the operation fails
		UUID uploadDynamic(Buffer& buf,D3D12_RESOURCE_DESC desc,bool isConstantBuffer = false);

		//copy the data from source buffer to target buffer
		//the source and target buffer must have transisted too corresponding state
		bool copyData(UUID target,UUID source,ID3D12GraphicsCommandList* cmdLis);

		//will insert a single resource barrier command immediately into the command list 
		//you can only change static buffer's state.generaly dynamic buffer's state is 
		//unchangable.
		bool immediateStateTransition(UUID token,D3D12_RESOURCE_STATES targetState,
			ID3D12GraphicsCommandList* cmdLis);

		bool releaseResource(UUID token);


		enum WRITE_STATE{
			NOT_FOUND = 0,
			SUCCESS = 1,
			INVAILD_BUFFER_SIZE = 2
		};

		WRITE_STATE write(UUID token,Buffer& buf);

		ID3D12Resource* getResource(UUID token);

		void initialize(ID3D12Device* dev);
		//the resource manager should be tick on every frame
		void tick(ID3D12GraphicsCommandList* cmdLis);

	private:

		ID3D12Device* mDevice;

		std::map<UUID, StaticResource> staticResource;
		std::map<UUID, DynamicResource> dynamicResource;

		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		std::vector<StaticResource*>        uploadWaitingList;
	};


};
