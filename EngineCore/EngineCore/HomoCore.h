#pragma once
#include "common.h"
#include "GPUBuffer.h"
#include "DynamicDescriptorHeap.h"
#include "CommandObjects.h"

namespace Core{
	class HomoEngineCore {
	public:
		static bool Initialize(UINT Width,UINT Height,HWND OutputWindow);
		static void OnResize(UINT Width,UINT Height);

		static IDXGISwapChain* GetSwapChain() {
			return core.mSwapChain.Get();
		}

		static CommandQueueManager& GetCommandQueue() {
			return core.cmdQueue;
		}

		static DXGI_FORMAT GetBackBuggerFormat() {
			return backBufferFormat;
		}

		static DXGI_FORMAT GetDepthBufferFormat() {
			return depthBufferFormat;
		}

		

		static ColorBuffer& GetRenderTarget(UINT backBufferIndex);
		static ColorBuffer& GetRenderTarget() { return GetRenderTarget(core.CurrBackBuffer); }

		static D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT BackBufferIndex);
		static D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() { return GetRenderTargetView(core.CurrBackBuffer); }

		static int GetCurrentFrameIndex() { return core.CurrBackBuffer; }

		static DepthBuffer& GetDepthBuffer() {
			return core.depthBuffer;
		}
		static D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferView() {
			return core.depthBuffer.GetDSV();
		}

		static const UINT BackBufferNum = 3;

		static D3D12_RECT GetSissorRect() {
			return core.SissorRect;
		}

		static D3D12_VIEWPORT GetViewPort() {
			return core.viewPort;
		}

		static void PresentAndSwapBackBuffer();
		
	private:
		HomoEngineCore():
		RTVHeap(nullptr),DSVHeap(nullptr){

		}
		static HomoEngineCore core;

		D3D12_RECT SissorRect;
		D3D12_VIEWPORT viewPort;
		
		ComPtr<IDXGISwapChain> mSwapChain;
		CommandQueueManager cmdQueue;
		HWND winHandle;
		UINT Width, Height;

		ColorBuffer BackBuffers[BackBufferNum];
		DepthBuffer depthBuffer;

		unique_ptr<StaticDescriptorHeap> RTVHeap;
		unique_ptr<StaticDescriptorHeap> DSVHeap;

		static const UINT HeapSize = 64;

		static const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		static const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		int CurrBackBuffer = 0;
	};
}