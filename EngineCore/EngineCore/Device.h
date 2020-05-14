#pragma once
#include "common.h"

namespace Core {
	//系统中只会有一个Devuce,统一通过一下接口
	namespace Device {

		//得到当前适配的设备
		/*inline ID3D12Device* GetCurrentDevice() {
			return nullptr;
		}

		//得到当前设备完成的fence值
		inline bool FenceIsCompleted(size_t FenceVal) {
			return true;
		}

		//初始化设备状态
		bool Initialize();

		//得到对应type的descirptor handle
		inline UINT GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE) {
			return 0;
		}*/
		
		
		ID3D12Device* GetCurrentDevice();

		bool Initialize();

		UINT GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE);

		
	}
}