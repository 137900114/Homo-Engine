#pragma once
#include "common.h"

namespace Core {
	class CommandQueueManager;
	//系统中只会有一个Device,统一通过一下接口

	class Device
	{

	public:
		static ID3D12Device* GetCurrentDevice();
		static IDXGIFactory* GetFactory();

		static bool CreateDevice();
		//void Release();

		static UINT GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE);

		~Device() {
			dev.mDevice->Release();
			dev.mFactory->Release();
		}
	private:
		static Device dev;

		ComPtr<ID3D12Device> mDevice;
		ComPtr<IDXGIFactory> mFactory;
		DXGI_ADAPTER_DESC adapter_info;
	};
	
}