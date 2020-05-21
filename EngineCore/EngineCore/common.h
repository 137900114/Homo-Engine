#pragma once
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")


#include <d3d12.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <wrl.h>
#include <memory>
#include <DirectXMath.h>

#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <algorithm>


template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using FenceVal = uint64_t;

#define GPU_VIRTUAL_ADDRESS_INVAILD (D3D12_GPU_VIRTUAL_ADDRESS)0
#define HANDLE_IS_INVAILD(handle) handle.ptr == GPU_VIRTUAL_ADDRESS_INVAILD
#define RESOURCE_STATE_INVAILD		(D3D12_RESOURCE_STATES)-1

#define COMMAND_LIST_TYPE_NUM 7

