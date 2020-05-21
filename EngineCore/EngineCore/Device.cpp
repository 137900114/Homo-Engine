#include "Device.h"
#include "trivial.h"
#include "DynamicDescriptorHeap.h"
#include "CommandObjects.h"

using namespace std;
using namespace Core;

Device Device::dev;

ID3D12Device* Device::GetCurrentDevice() {
	return dev.mDevice.Get();
}

IDXGIFactory* Device::GetFactory() {
	return dev.mFactory.Get();
}

bool Device::CreateDevice() {
	CreateDXGIFactory(IID_PPV_ARGS(&dev.mFactory));

	ComPtr<IDXGIAdapter> adapter = nullptr;
	HRESULT hr = E_FAIL;
	UINT index = 0;
	
	while (dev.mFactory->EnumAdapters(index++,&adapter) != DXGI_ERROR_NOT_FOUND) {
		hr = D3D12CreateDevice(adapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&dev.mDevice)
		);
		if (!FAILED(hr)) {
			adapter->GetDesc(&dev.adapter_info);
			break;
		}
	}

	ASSERT_HR(hr,"Device::Initialize no device suppert feature level D3D_FEATURE_LEVEL_11_0");
	

	return true;
}

UINT Device::GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE type) {
	return dev.mDevice->GetDescriptorHandleIncrementSize(type);
}