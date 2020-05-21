#pragma once
#include "common.h"

namespace Core {
	class CommandBuffer;

	class Resource {
		friend CommandBuffer;
	public:
		virtual ~Resource() { Release(); }

		Resource() :
		mResource(nullptr),
		mAddress(GPU_VIRTUAL_ADDRESS_INVAILD),
		mCurrentState(D3D12_RESOURCE_STATE_COMMON){

		}

		Resource(ID3D12Resource* resource, D3D12_RESOURCE_STATES initState):
		mResource(resource),
		mAddress(resource->GetGPUVirtualAddress()),
		mCurrentState(initState){}


		void Release() {
			mResource = nullptr;
			mAddress = GPU_VIRTUAL_ADDRESS_INVAILD;
			mCurrentState = D3D12_RESOURCE_STATE_COMMON;
		}

		ID3D12Resource* Get() const { return mResource.Get(); }
		//const ID3D12Resource* Get() const { return mResource.Get(); }

		D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const { return mAddress; }

	protected:

		ComPtr<ID3D12Resource> mResource;
		D3D12_GPU_VIRTUAL_ADDRESS mAddress;
		D3D12_RESOURCE_STATES mCurrentState;
	};

}
