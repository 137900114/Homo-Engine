#pragma once

#include "d3dCommon.h"
#include "uuid.h"
#include <map>
#include "Buffer.h"
#include "Image.h"


namespace Game {

	class D3DResourceManager {
		enum STATIC_RESOURCE_TYPE{
			BUFFER,
			TEXTURE2D,
			TEXTURE2D_ARRAY
		};


		struct StaticResource{
			bool uploaded;
			ComPtr<ID3D12Resource> buffer;
			ComPtr<ID3D12Resource> uploadBuffer;
			union{
				void* bufferWriter;
				
				struct{
					void* data;
					size_t rowPitch;
					size_t slicePitch;
				} textureData;

				struct {
					SubTextureDescriptor* subTex;
					size_t subTexNum;
				} textureArray;
			};
			D3D12_RESOURCE_STATES currState;

			STATIC_RESOURCE_TYPE  type;
		};

		//upload buffer's will always be D3D12_RESOURCE_STATE_GENERIC_READ
		struct DynamicResource {
			ComPtr<ID3D12Resource> buffer;
			size_t size;
			void* bufferWriter;
		};

		
	public:

		D3DResourceManager():mDevice(nullptr) {}

		//the resource uploaded staticly can't be changed,if the resource is a constant buffer we have to 
		//aliase it to 256 so we need to inform the resource manager by passing a true to parameter 'isConstantBuffer'
		//the function will return a invalid uuid when the operation fails
		//this function only upload 1D Buffer data,to upload the 2D texture data please call uploadTexture function
		UUID uploadStaticBuffer(void* data,size_t size,D3D12_RESOURCE_STATES initState,bool isConstantBuffer = false);
		//the resources uploaded dynamicly can be changed by calling write. if the resource is a constant buffer we have to
		//aliase it to 256 so we need to inform the resource manager by passing a true to parameter 'isConstantBuffer'
		//the function will return a invalid uuid when the operation fails
		UUID uploadDynamic(void* data, size_t size,D3D12_RESOURCE_DESC desc,bool isConstantBuffer = false);

		//upload texture to Resource Manager.Currently we don't support mipmap.We will support mipmap 
		//some times later
		UUID uploadTexture(void* data,size_t width,size_t height,D3D12_RESOURCE_STATES initState,
			size_t rowPitch,size_t mipLevel = 1,DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

		UUID uploadCubeTexture(size_t width,size_t height,D3D12_RESOURCE_STATES initState,
			SubTextureDescriptor* subTex,DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

		//upload texture to Resource Manager.Currently we don't support mipmap.We will support mipmap 
		//some times later
		UUID uploadTexture(Texture& image, D3D12_RESOURCE_STATES initState, size_t mipLevel = 1);

		//the resource uploaded staticly can't be changed,if the resource is a constant buffer we have to 
		//aliase it to 256 so we need to inform the resource manager by passing a true to parameter 'isConstantBuffer'
		//the function will return a invalid uuid when the operation fails
		inline UUID uploadStaticBuffer(Buffer& data,D3D12_RESOURCE_STATES initState,bool isConstantBuffer = false) {
			return uploadStaticBuffer(data.data,data.size,initState,isConstantBuffer);
		}

		inline UUID uploadDynamic(Buffer& data,D3D12_RESOURCE_DESC desc,bool isConstantBuffer = false) {
			return uploadDynamic(data.data, data.size, desc, isConstantBuffer);
		}


		//copy the data from source buffer to target buffer
		//the source and target buffer must have transisted too corresponding state
		bool copyData(UUID target,UUID source,ID3D12GraphicsCommandList* cmdLis);

		//will insert a single resource barrier command immediately into the command list 
		//you can only change static buffer's state.generaly dynamic buffer's state is 
		//unchangable.
		bool immediateStateTransition(UUID token,D3D12_RESOURCE_STATES targetState,
			ID3D12GraphicsCommandList* cmdLis);

		bool releaseResource(UUID token);


		enum WRITE_STATE{
			NOT_FOUND = 0,
			SUCCESS = 1,
			INVAILD_BUFFER_SIZE = 2
		};

		WRITE_STATE write(UUID token, void* data, size_t size);

		ID3D12Resource* getResource(UUID token);

		void initialize(ID3D12Device* dev);
		//the resource manager should be tick on every frame
		void tick(ID3D12GraphicsCommandList* cmdLis);

		void finalize();

	private:

		ID3D12Device* mDevice;

		std::map<UUID, StaticResource> staticResource;
		std::map<UUID, DynamicResource> dynamicResource;

		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		std::vector<StaticResource*>        uploadWaitingList;
		std::vector<StaticResource*>        uploadImageWaitingList;
	};


};
