#include "GPUBuffer.h"
#include "Device.h"
#include "trivial.h"
#include "CommandObjects.h"
#include "DDSTextureLoader.h"
using namespace Core;

D3D12_RESOURCE_DESC GPUBuffer::DescribeResource() {
	D3D12_RESOURCE_DESC rdesc;
	rdesc.Format = DXGI_FORMAT_UNKNOWN;
	rdesc.DepthOrArraySize = 1;
	rdesc.Alignment = 0;
	rdesc.MipLevels = 1;
	rdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	rdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rdesc.Height = 1;
	rdesc.Width = buffer_size;
	rdesc.SampleDesc.Count = 1;
	rdesc.SampleDesc.Quality = 0;

	return rdesc;
}


void GPUBuffer::Create(UINT element_size,UINT element_num,void* init_data) {
	
	Resource::Release();

	this->element_num = element_num;
	this->element_size = element_size;
	this->buffer_size = element_size * element_num;


	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&DescribeResource(),
		mCurrentState,
		nullptr,
		IID_PPV_ARGS(&mResource)
	);

	ASSERT_HR(hr, "GPUBuffer::Create : Fail to create resource");

	mAddress = mResource->GetGPUVirtualAddress();

	
	if (init_data) {
		CommandBuffer::Upload2Buffer(init_data, buffer_size, *this);
	}
}

void GPUBuffer::CreatePlaced(UINT element_size,UINT element_num,ID3D12Heap* Heap,UINT heap_offset,
	void* init_data) {
	

	Resource::Release();

	this->element_num = element_num;
	this->element_size = element_size;
	this->buffer_size = element_num * element_size;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreatePlacedResource(
		Heap,
		heap_offset,
		&DescribeResource(),
		mCurrentState,
		nullptr,
		IID_PPV_ARGS(&mResource));

	ASSERT_HR(hr, "GPUBuffer::Create : Fail to create resource");

	mAddress = mResource->GetGPUVirtualAddress();

	
	if (init_data) {
		CommandBuffer::Upload2Buffer(init_data, buffer_size, *this);
	}
}

void GPUBuffer::CreateSRVUAV(D3D12_CPU_DESCRIPTOR_HANDLE srv, D3D12_CPU_DESCRIPTOR_HANDLE uav) {

	ID3D12Device* dev = Device::GetCurrentDevice();
	
	if (srv.ptr != GPU_VIRTUAL_ADDRESS_INVAILD) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = element_num;
		srvDesc.Buffer.StructureByteStride = element_size;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.FirstElement = 0;

		dev->CreateShaderResourceView(mResource.Get(), &srvDesc, srv);
		SRVHandle = srv;
	}

	if (uav.ptr != GPU_VIRTUAL_ADDRESS_INVAILD) {
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = element_num;
		uavDesc.Buffer.StructureByteStride = element_size;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		uavDesc.Buffer.CounterOffsetInBytes = 0;

		dev->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, uav);
		UAVHandle = uav;
	}
}


void GPUBuffer::CopyData2Buffer(void* data,ID3D12Resource** Upload,ID3D12GraphicsCommandList* cmdLis) {
	D3D12_RESOURCE_DESC uploadDesc = DescribeResource();
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(Upload)
	);

	
	if(mCurrentState != D3D12_RESOURCE_STATE_COPY_DEST)
		cmdLis->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(
				mResource.Get(), mCurrentState, D3D12_RESOURCE_STATE_COPY_DEST
			));
	
	D3D12_SUBRESOURCE_DATA subres;
	subres.pData = data;
	subres.RowPitch = buffer_size;
	subres.SlicePitch = buffer_size;

	UpdateSubresources<1>(cmdLis,mResource.Get(),*Upload,0,0,1,&subres);
	
	if (mCurrentState != D3D12_RESOURCE_STATE_COPY_DEST) {
		cmdLis->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, mCurrentState
		));
	}

}


void UploadBuffer::Upload2Buffer(void* src,UINT num,UINT indexOffset) {
	ASSERT_WARNING(indexOffset + num > element_num,"UploadBuffer::Upload2Buffer : the data to upload is outof range");

	void* target = reinterpret_cast<char*>(res_p) + element_size * indexOffset;
	memcpy(target, src, num * element_size);
}

void UploadBuffer::Create(UINT element_size, UINT element_num, void* /*initial_data*/) {
	Resource::Release();

	this->element_num = element_num;
	this->element_size = element_size;
	this->buffer_size = element_num * element_size;
	this->mCurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;
	
	D3D12_RESOURCE_DESC rdesc = DescribeResource();
	rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&rdesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(mResource.GetAddressOf())
	);

	ASSERT_HR(hr, "UploadBuffer::Create : fail to create buffer");

	mAddress = mResource->GetGPUVirtualAddress();

	Map();
}

void UploadBuffer::CreatePlaced(UINT element_size, UINT element_num, ID3D12Heap* heap, UINT heap_offset,
	void* /*initial_data*/) {
	Resource::Release();

	this->element_num = element_num;
	this->element_size = element_size;
	this->buffer_size = element_num * element_size;
	this->mCurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;

	D3D12_RESOURCE_DESC rdesc = DescribeResource();
	rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreatePlacedResource(
		heap,heap_offset,
		&rdesc,D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,IID_PPV_ARGS(mResource.GetAddressOf())
	);

	ASSERT_HR(hr, "UploadBuffer::CreatePlaced : fail to create buffer");

	mAddress = mResource->GetGPUVirtualAddress();
}

D3D12_RESOURCE_DESC PixelBuffer::DescribeResource(UINT16 mipNum,D3D12_RESOURCE_FLAGS flag, UINT sampleCount, UINT sampleQuality) {
	D3D12_RESOURCE_DESC desc;
	
	desc.Format = format;
	desc.Alignment = 0;
	desc.DepthOrArraySize = arraySize;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = flag;
	desc.Height = height;
	desc.Width = width;
	desc.MipLevels = mipNum;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	desc.SampleDesc.Count = sampleCount;
	desc.SampleDesc.Quality = sampleQuality;

	return desc;
}

//readbackbuffer can't and don't need to be initialized so the initial data is useless
void ReadBackBuffer::Create(UINT element_size, UINT element_num, void* /*initial_data*/) {
	Resource::Release();

	this->element_size = element_size;
	this->element_num = element_num;
	this->buffer_size = element_num * element_size;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&DescribeResource(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,IID_PPV_ARGS(&mResource));

	ASSERT_HR(hr, "ReadBackBuffer::Create : fail to create resource");

	mCurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
	mAddress = mResource->GetGPUVirtualAddress();

	//readbackbuffer can't and don't need to be initialized so the initial data is useless
}

//readbackbuffer can't and don't need to be initialized so the initial data is useless
void ReadBackBuffer::CreatePlaced(UINT element_size, UINT element_num,
	ID3D12Heap* heap, UINT heap_offset,void* /*initial_data*/) {

	Resource::Release();

	this->element_size = element_size;
	this->element_num = element_num;
	this->buffer_size = element_num * element_size;

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreatePlacedResource(
		heap, heap_offset,
		&DescribeResource(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(&mResource)
	);

	ASSERT_HR(hr,"ReadBackBuffer::CreatePlaced : fail to create resource");

	mCurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
	mAddress = mResource->GetGPUVirtualAddress();
	//readbackbuffer can't and don't need to be initialized so the initial data is useless
}

void ReadBackBuffer::Write2CPU(void* Dest,void (*writer)(void*,void*,UINT)) {
	ASSERT_WARNING(!mResource, "ReadBackBuffer::Write2CPU : You should create the resource before writing");
	if (!res_p)  Map();

	ASSERT_WARNING(!res_p, "ReadBackBuffer::Write2CPU : Fail to map resource");
	if (!writer)
		memcpy(Dest, res_p, buffer_size);
	else
		writer(Dest, res_p, buffer_size);
}

//color buffer don't use mipmap(There is only one mipmap level)
void ColorBuffer::Create(UINT width,UINT height,DXGI_FORMAT format,UINT sampleCount,UINT sampleQuality) {
	Resource::Release();

	this->width = width;
	this->height = height;
	this->arraySize = 1;
	this->format = format;
	this->sampleCount = sampleCount;

	D3D12_RESOURCE_DESC desc = DescribeResource(1,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		sampleCount, sampleQuality);

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&mResource)
	);

	ASSERT_HR(hr, "ColorBuffer::Create : fail to create resource");

	mCurrentState = D3D12_RESOURCE_STATE_COMMON;
}

void ColorBuffer::CreateRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};

	desc.Format = format;
	 if (arraySize > 1) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.PlaneSlice = 0;
	}else if (sampleCount > 1) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	}
	else {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;
	}

	 ID3D12Device* dev = Device::GetCurrentDevice();
	 dev->CreateRenderTargetView(mResource.Get(), &desc, handle);

	 mRtv = handle;
}

void ColorBuffer::CreateShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};

	if (arraySize > 1) {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.MostDetailedMip = 0;
		desc.Texture2DArray.PlaneSlice = 0;
	}
	else if(sampleCount > 1){
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.PlaneSlice = 0;
	}

	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateShaderResourceView(
		mResource.Get(), &desc, handle
	);

	mSrv = handle;
}

void ColorBuffer::CreateUnorderedAccessView(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	ASSERT_WARNING(sampleCount > 1,"ColorBuffer::CreateUnorderedAccessView : texture use msaa can't create unordered access view");

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};

	if (arraySize > 1) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = arraySize;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.PlaneSlice = 0;
	}
	else {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;
	}

	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateUnorderedAccessView(mResource.Get(),nullptr, &desc, handle);

	mUav = handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE ColorBuffer::GetRTV()const {
	return mRtv;
}

D3D12_CPU_DESCRIPTOR_HANDLE ColorBuffer::GetSRV() const {
	return mSrv;
}

D3D12_CPU_DESCRIPTOR_HANDLE ColorBuffer::GetUAV() const {
	return mUav;
}

//color buffer don't use mipmap(There is only one mipmap level)
void ColorBuffer::CreateArray(UINT Width, UINT Height, UINT ArraySize, DXGI_FORMAT Format, UINT sampleCount, UINT sampleQuality) {
	Resource::Release();
	
	this->width = Width;
	this->height = Height;
	this->arraySize = ArraySize;
	this->format = Format;
	this->sampleCount = sampleCount;

	D3D12_RESOURCE_DESC desc = DescribeResource(1,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		sampleCount, sampleQuality);

	ID3D12Device* dev = Device::GetCurrentDevice();
	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&mResource)
	);

	ASSERT_HR(hr, "ColorBuffer::CreateArray : fail to create resource");

	mCurrentState = D3D12_RESOURCE_STATE_COMMON;
}

void DepthBuffer::Create(UINT Width, UINT Height) {
	Resource::Release();

	this->width = Width;
	this->height = Height;
	this->format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	this->arraySize = 1;
	this->mCurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	D3D12_RESOURCE_DESC desc = DescribeResource(1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 1, 0);
	ID3D12Device* dev = Device::GetCurrentDevice();

	D3D12_CLEAR_VALUE val = {};
	val.DepthStencil.Depth = 1.f;
	val.DepthStencil.Stencil = 0.f;
	val.Format = format;

	HRESULT hr = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc, mCurrentState, &val,
		IID_PPV_ARGS(&mResource)
	);

	ASSERT_HR(hr, "DepthBuffer::Create : fail to create resource");
}

void DepthBuffer::CreateDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv) {
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	desc.Flags = D3D12_DSV_FLAG_NONE;
	desc.Format = format;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateDepthStencilView(mResource.Get(),&desc, dsv);

	mDsv = dsv;
}

void DepthBuffer::CreateDepthShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srv) {
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.PlaneSlice = 0;

	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateShaderResourceView(mResource.Get(),&desc, srv);

	mDepthSrv = srv;
}

void DepthBuffer::CreateStencilShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srv) {
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.PlaneSlice = 0;
	
	ID3D12Device* dev = Device::GetCurrentDevice();
	dev->CreateShaderResourceView(mResource.Get(), &desc, srv);

	mStencilSrv = srv;
}

void ColorBuffer::CreateFromSwapChain(IDXGISwapChain* sc,UINT index) {
	Resource::Release();

	ID3D12Resource* BaseRes;
	HRESULT hr = sc->GetBuffer(index,IID_PPV_ARGS(&BaseRes));
	ASSERT_HR(hr,"ColorBuffer::CreateFromSwapChain : Fail to get buffer from swap chain.\n"
		"May be you should check whether the index is outof bounary!");


	mResource.Attach(BaseRes);
	D3D12_RESOURCE_DESC desc = BaseRes->GetDesc();
	this->width = desc.Width;
	this->height = desc.Height;
	this->arraySize = desc.DepthOrArraySize;
	//we can't get the resource's gpu virtual address if the resource's type is not buffer
	//this->mAddress = mResource->GetGPUVirtualAddress();
	this->format = desc.Format;
	this->sampleCount = desc.SampleDesc.Count;
	//resource from swap chain 's intial state must be PRESENT
	this->mCurrentState = D3D12_RESOURCE_STATE_PRESENT;
}