#include "GPUBuffer.h"
#include "Device.h"
#include "trivial.h"
#include "CommandObjects.h"
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
