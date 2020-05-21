#pragma once
#include "d3dUtil.h"

class SobelFilter
{
public:
	void Resize(UINT width,UINT height);

	void CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outPutCPU,
		D3D12_GPU_DESCRIPTOR_HANDLE& outPutGPU,
		UINT desHandleSize);

	void CreateDescriptor();

	void Excute(ID3D12GraphicsCommandList* cmdLis,ID3D12Resource* input);

	SobelFilter(UINT with, UINT height,ID3D12Device* dev,
		DXGI_FORMAT format);

	
	bool CreateRootSig();

	bool CreatePSO(const wchar_t* fileName,const char* entryPoint);
private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> SobelPSO = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> SobelRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob>            CS = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource>      outPut = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>      inputCopy = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE inPutCPUSRV;
	D3D12_GPU_DESCRIPTOR_HANDLE inPutGPUSRV;

	D3D12_CPU_DESCRIPTOR_HANDLE outPutCPUUAV;
	D3D12_GPU_DESCRIPTOR_HANDLE outPutGPUUAV;

	DXGI_FORMAT mFormat;

	UINT width = 0, height = 0;
	ID3D12Device* dev;

	void CreateResource();
};

