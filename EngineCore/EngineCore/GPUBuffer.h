#pragma once
#include "Resource.h"

namespace  Core {
	class GPUBuffer : public  Resource {
	public:
		virtual void Create(UINT element_size, UINT element_num, void* init_data = nullptr);

		virtual void CreatePlaced(UINT element_size, UINT element_num, ID3D12Heap* heap, UINT heap_offset,
			void* init_data = nullptr);

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() {

			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = mAddress;
			vbv.SizeInBytes = buffer_size;
			vbv.StrideInBytes = element_size;

			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() {
			D3D12_INDEX_BUFFER_VIEW ibv;

			ibv.BufferLocation = mAddress;
			ibv.Format = DXGI_FORMAT_R16_UINT;
			ibv.SizeInBytes = buffer_size;

			return ibv;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return UAVHandle; }
		const D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return SRVHandle; }

		const D3D12_GPU_VIRTUAL_ADDRESS  GetCBV() const { return mAddress; }
		const D3D12_GPU_VIRTUAL_ADDRESS  GetCBV(UINT index) const { return mAddress + index * element_size; }

		const UINT GetElementNum() const { return element_num; }

		void CreateSRVUAV(D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle, D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle);

		//不推荐使用的方法
		void CopyData2Buffer(void* data, ID3D12Resource** UploadBuffer, ID3D12GraphicsCommandList* cmdLis);

		GPUBuffer() {
			element_size = 0;
			element_num = 0;
			buffer_size = 0;

			UAVHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			SRVHandle.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
		}

		~GPUBuffer() { Release(); }

	protected:

		D3D12_RESOURCE_DESC DescribeResource();

		UINT element_size;
		UINT element_num;
		UINT buffer_size;

		D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
	};


	class UploadBuffer : public GPUBuffer {
	public:
		virtual void Create(UINT element_size, UINT element_num, void* initial_data = nullptr) override;
		virtual void CreatePlaced(UINT element_size, UINT element_num, ID3D12Heap* heap,
			UINT heap_offset, void* initial_data = nullptr) override;

		void Upload2Buffer(void* src, UINT num, UINT indexOffset);

		~UploadBuffer() { if (!res_p) UnMap(); Release(); }
		UploadBuffer() { res_p = nullptr; }
	private:
		void* res_p;

		void Map() { mResource->Map(0, nullptr, &res_p); }
		void UnMap() { mResource->Unmap(0, nullptr); }

	};

	//ReadBackBuffer Can only read back from gpu to cpu
	class ReadBackBuffer : public GPUBuffer {
	public:
		void CreateSRVUAV(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE) = delete;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const = delete;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const = delete;

		const D3D12_GPU_VIRTUAL_ADDRESS GetCBV() const = delete;
		const D3D12_GPU_VIRTUAL_ADDRESS GetCBV(UINT) const = delete;

		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() = delete;

		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() = delete;

		virtual void Create(UINT element_size, UINT element_num, void* initial_data = nullptr) override;
		virtual void CreatePlaced(UINT element_size, UINT element_num, ID3D12Heap* heap, UINT heap_offset
			, void* initial_data = nullptr) override;

		//the first parameter of writer is the write destination,the second is the source
		void Write2CPU(void* Dest, void (*writer)(void*, void*, UINT) = nullptr);

		ReadBackBuffer() { res_p = nullptr; }
		~ReadBackBuffer() { if (!res_p) UnMap(); Release(); }
	private:
		void* res_p;

		void Map() { mResource->Map(0, nullptr, &res_p); }
		void UnMap() { mResource->Unmap(0, nullptr); }
	};

	class PixelBuffer : public Resource {

		UINT Width() const { return width; }
		UINT Height() const { return height; }
		UINT ArraySize() const { return arraySize; }
		DXGI_FORMAT Format() const { return format; }

	protected:
		PixelBuffer() :width(0), height(0), format(DXGI_FORMAT(-1)), arraySize(0) {}

		D3D12_RESOURCE_DESC DescribeResource(UINT16 mipNum, D3D12_RESOURCE_FLAGS flag, UINT sampleCount, UINT sampleQuality);

		UINT width;
		UINT height;
		UINT arraySize;
		DXGI_FORMAT format;
	};

	//depth buffer don't support msaa and mipmap.The default format is d24_unorm_s8_uint
	class DepthBuffer : public PixelBuffer {
	public:
		DepthBuffer() {
			mDsv.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			mDepthSrv.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			mStencilSrv.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
		}

		void Create(UINT Width, UINT Height);

		void CreateDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv);
		void CreateDepthShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srv);
		void CreateStencilShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srv);

		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV()const { return mDsv; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const { return mDepthSrv; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const { return mStencilSrv; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE mDsv;
		D3D12_CPU_DESCRIPTOR_HANDLE mDepthSrv;
		D3D12_CPU_DESCRIPTOR_HANDLE mStencilSrv;
	};

	//ColorBuffer don't support mipmap
	class ColorBuffer : public PixelBuffer {
	public:
		ColorBuffer() {
			mRtv.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			mUav.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			mSrv.ptr = GPU_VIRTUAL_ADDRESS_INVAILD;
			sampleCount = 1;
		}

		void Create(UINT Width, UINT Height, DXGI_FORMAT Format, UINT sampleCount = 1, UINT sampleQuality = 0);
		void CreateArray(UINT Width, UINT Height, UINT ArraySize, DXGI_FORMAT Format, UINT sampleCount = 1, UINT sampleQuality = 0);

		void CreateFromSwapChain(IDXGISwapChain* sc,UINT index);

		void CreateRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
		void CreateUnorderedAccessView(D3D12_CPU_DESCRIPTOR_HANDLE uav);
		void CreateShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srv);

		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE mRtv;
		D3D12_CPU_DESCRIPTOR_HANDLE mUav;
		D3D12_CPU_DESCRIPTOR_HANDLE mSrv;
		UINT sampleCount;
	};
}