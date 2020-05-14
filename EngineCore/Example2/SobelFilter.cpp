#include "SobelFilter.h"


template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

SobelFilter::SobelFilter(UINT width,UINT height,ID3D12Device* dev,DXGI_FORMAT format):
width(width),height(height),dev(dev),mFormat(format){
	CreateResource();
}

bool SobelFilter::CreateRootSig() {
	if (dev == nullptr) return false;
	CD3DX12_DESCRIPTOR_RANGE in;
	in.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE out;
	out.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_ROOT_PARAMETER para[2];
	para[0].InitAsDescriptorTable(1, &in);
	para[1].InitAsDescriptorTable(1, &out);


	CD3DX12_ROOT_SIGNATURE_DESC desc(_countof(para),para,0,nullptr);

	ComPtr<ID3DBlob>  rootSigBlob = nullptr;
	ComPtr<ID3DBlob>  errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
	if (FAILED(hr)) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return false;
	}

	dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&SobelRootSig));
	return true;
}

bool SobelFilter::CreatePSO(const wchar_t* fileName,const char* entryPoint) {
	if (dev == nullptr) return false;
	CS = d3dUtil::CompileShader(fileName, nullptr, entryPoint, "cs_5_0");

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = SobelRootSig.Get();
	desc.CS = {
		CS->GetBufferPointer(),
		CS->GetBufferSize()
	};
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	dev->CreateComputePipelineState(&desc, IID_PPV_ARGS(&SobelPSO));
	return true;
}

void SobelFilter::Resize(UINT width,UINT height) {
	if (this->width != width || this->height != height) {
		this->width = width;
		this->height = height;

		CreateResource();
		CreateDescriptor();
	}
}

void SobelFilter::CreateResource() {
	D3D12_RESOURCE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	
	desc.Width = width;
	desc.Height = height;

	desc.DepthOrArraySize = 1;
	desc.Format = mFormat;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(dev->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	D3D12_HEAP_FLAG_NONE,
	&desc,D3D12_RESOURCE_STATE_COMMON,
	nullptr,IID_PPV_ARGS(&outPut)));

	ThrowIfFailed(dev->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	D3D12_HEAP_FLAG_NONE,
	&desc,D3D12_RESOURCE_STATE_COMMON,
	nullptr,IID_PPV_ARGS(&inputCopy)));
}

void SobelFilter::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle,UINT descriptorSize) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE ccHandle(cpuHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gcHandle(gpuHandle);

	inPutCPUSRV = ccHandle.Offset(1,descriptorSize);
	inPutGPUSRV = gcHandle.Offset(1,descriptorSize);
	
	
	outPutCPUUAV = ccHandle.Offset(1, descriptorSize);
	outPutGPUUAV = gcHandle.Offset(1, descriptorSize);

	cpuHandle = ccHandle;
	gpuHandle = gcHandle;

	CreateDescriptor();
}

void SobelFilter::CreateDescriptor() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = mFormat;
	uavDesc.Texture2D.MipSlice = 0;

	dev->CreateShaderResourceView(inputCopy.Get(),&srvDesc,inPutCPUSRV);
	dev->CreateUnorderedAccessView(outPut.Get(), nullptr, &uavDesc, outPutCPUUAV);
}

void SobelFilter::Excute(ID3D12GraphicsCommandList* cmdLis,ID3D12Resource* input) {

	cmdLis->SetPipelineState(SobelPSO.Get());
	cmdLis->SetComputeRootSignature(SobelRootSig.Get());

	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE));
	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputCopy.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST));

	cmdLis->CopyResource(inputCopy.Get(), input);


	cmdLis->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(inputCopy.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ) );
	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(outPut.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	UINT XGNum = width / 16;
	UINT YGNum = height / 16;



	cmdLis->SetComputeRootDescriptorTable(0,inPutGPUSRV);
	cmdLis->SetComputeRootDescriptorTable(1,outPutGPUUAV);

	cmdLis->Dispatch(XGNum, YGNum, 1);

	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST));
	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(outPut.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdLis->CopyResource(input, outPut.Get());

	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(outPut.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COMMON));
	cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputCopy.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_COMMON));
}