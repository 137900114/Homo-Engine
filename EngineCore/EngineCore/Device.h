#pragma once
#include "common.h"

namespace Core {
	//ϵͳ��ֻ����һ��Devuce,ͳһͨ��һ�½ӿ�
	namespace Device {

		//�õ���ǰ������豸
		/*inline ID3D12Device* GetCurrentDevice() {
			return nullptr;
		}

		//�õ���ǰ�豸��ɵ�fenceֵ
		inline bool FenceIsCompleted(size_t FenceVal) {
			return true;
		}

		//��ʼ���豸״̬
		bool Initialize();

		//�õ���Ӧtype��descirptor handle
		inline UINT GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE) {
			return 0;
		}*/
		
		
		ID3D12Device* GetCurrentDevice();

		bool Initialize();

		UINT GetDescriptorIncreamentHandleOffset(D3D12_DESCRIPTOR_HEAP_TYPE);

		
	}
}