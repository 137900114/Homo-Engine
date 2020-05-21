#pragma once
#include "HomoCore.h"
#include "Device.h"
#include "trivial.h"

using namespace Core;

HomoEngineCore HomoEngineCore::core;

bool HomoEngineCore::Initialize(UINT Width,UINT Height,HWND Handle) {
	Device::CreateDevice();

	core.cmdQueue.Initialize();
	CommandBufferManager::Initialize(&core.cmdQueue);

	core.Width = Width, core.Height = Height;
	core.winHandle = Handle;

	DXGI_SWAP_CHAIN_DESC scDesc = {};
	scDesc.BufferDesc.Width = Width;
	scDesc.BufferDesc.Height = Height;
	scDesc.BufferDesc.Format = backBufferFormat;
	scDesc.BufferDesc.RefreshRate.Numerator = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.BufferCount = BackBufferNum;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scDesc.OutputWindow = core.winHandle;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Windowed = true;
	

	HRESULT hr =Device::GetFactory()->CreateSwapChain(
		core.cmdQueue.GetGraphicQueue().GetCommandQueue(),
		&scDesc,
		&core.mSwapChain
	);

	if (FAILED(hr)) {
		OutputDebugStringA("Fail to create swap chain ");
		__debugbreak();
	}

	core.RTVHeap = make_unique<StaticDescriptorHeap>(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,HeapSize,D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	core.DSVHeap = make_unique<StaticDescriptorHeap>(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,HeapSize,D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	OnResize(Width,Height);

	return true;

}

void HomoEngineCore::OnResize(UINT Width,UINT Height) {
	core.Width = Width;
	core.Height = Height;

	core.CurrBackBuffer = 0;

	CommandQueue& gQueue = core.cmdQueue.GetGraphicQueue();
	core.cmdQueue.WaitForFence(gQueue.IncreaseAndSignalFenceValue());

	for (int i = 0; i != BackBufferNum; i++) {
		core.BackBuffers[i].Release();
	}
	core.depthBuffer.Release();

	core.mSwapChain->ResizeBuffers(
		BackBufferNum,Width, Height,backBufferFormat,DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	for (int i = 0; i != BackBufferNum; i++) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtv =  core.BackBuffers[i].GetRTV();
		if (rtv.ptr == GPU_VIRTUAL_ADDRESS_INVAILD) {
			rtv = core.RTVHeap->Allocate().cpuHandle;
		}
		core.BackBuffers[i].CreateFromSwapChain(GetSwapChain(), i);
		core.BackBuffers[i].CreateRenderTargetView(rtv);
	}
	
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = core.depthBuffer.GetDSV();
	if (dsv.ptr == GPU_VIRTUAL_ADDRESS_INVAILD) {
		dsv = core.DSVHeap->Allocate().cpuHandle;
	}
	core.depthBuffer.Create(Width, Height);
	core.depthBuffer.CreateDepthStencilView(dsv);

	core.SissorRect.left = 0;
	core.SissorRect.right = Width;
	core.SissorRect.top = 0;
	core.SissorRect.bottom = Height;

	core.viewPort.Height = Height;
	core.viewPort.Width = Width;
	core.viewPort.MaxDepth = 1.0f;
	core.viewPort.MinDepth = 0.0f;
	core.viewPort.TopLeftX = 0;
	core.viewPort.TopLeftY = 0;

}

ColorBuffer& HomoEngineCore::GetRenderTarget(UINT backBufferIndex ) {
	ASSERT_WARNING(backBufferIndex >= BackBufferNum,"HomoEngineCore::GetRenderTarget : index out of bounary ");
	return core.BackBuffers[backBufferIndex];
}

D3D12_CPU_DESCRIPTOR_HANDLE HomoEngineCore::GetRenderTargetView(UINT backBufferIndex) {
	ASSERT_WARNING(backBufferIndex >= BackBufferNum,"HomoEngineCore::GetRenderTargetView : index out of bounary ");
	return core.BackBuffers[backBufferIndex].GetRTV();
}

void HomoEngineCore::PresentAndSwapBackBuffer() {
	core.mSwapChain->Present(0,0);
	core.CurrBackBuffer = (core.CurrBackBuffer + 1) % core.BackBufferNum;
}