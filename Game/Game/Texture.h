#pragma once
#include "Buffer.h"
#include "uuid.h"

namespace Game {
	//Texture is a buffer type object which means that 
	//outside should use pointer reference it rather than making copy of it
	//because the copy operation will make the original buffer invalid
	//
	enum class TEXTURE_TYPE{
		CUBE,
		TEXTURE2D,
		INVAILD
	};

	struct SubTextureHeader {
		uint32_t subResourceNum;
		uint32_t mipMapNum;
		uint32_t arrayNum;
	};

	struct SubTextureDescriptor {
		void* resourceData;
		int64_t rowPitch;
		int64_t slicePitch;
	};

	//Helps the user fill the CudeMap array
	struct CubeMapFileArray {
		const char* right;
		const char* left;
		const char* up;
		const char* down;
		const char* front;
		const char* back;
	};

	//If the Texture is only a single 2D texture with only 1 mipmap level.The data only contians the
	//rgba texture data.Otherwise the data buffer will start with a array of sub texture descriptors which
	//describes how the texture data organized in memory.'subDataDescriptor' pointer helps you access these data.
	struct Texture{
		size_t width, height,rowPitch;
		Buffer data;
		//you can use a uuid to represent data uploaded to gpu
		UUID gpuDataToken;
		TEXTURE_TYPE type;

		SubTextureHeader subDataHead;
		SubTextureDescriptor* subDataDescriptor;

		Texture& operator=(Texture&& image) noexcept {
			data = std::move(image.data);
			width = image.width, height = image.height;
			rowPitch = image.rowPitch;
			type = image.type;
			subDataHead = image.subDataHead;
			subDataDescriptor = image.subDataDescriptor;

			image.width = 0, image.height = 0;
			image.rowPitch = 0;
			image.type = TEXTURE_TYPE::INVAILD;

			image.subDataHead = {0,0,0};
			image.subDataDescriptor = nullptr;
			
			this->gpuDataToken = std::move(image.gpuDataToken);
			return *this;
		}

		Texture(Texture&& image) noexcept {
			data = std::move(image.data);
			width = image.width, height = image.height;
			rowPitch = image.rowPitch;
			type = image.type;
			subDataHead = image.subDataHead;
			subDataDescriptor = image.subDataDescriptor;
			
			image.width = 0, image.height = 0;
			image.rowPitch = 0;
			image.type = TEXTURE_TYPE::INVAILD;

			image.subDataHead = { 0,0,0 };
			image.subDataDescriptor = nullptr;

			this->gpuDataToken = std::move(image.gpuDataToken);
		}

		//we make a copy of the original image if we use normal renference 
		Texture& operator=(const Texture& image) {
			data = image.data;
			width = image.width, height = image.height;
			rowPitch = image.rowPitch;
			type = image.type;
			subDataHead = image.subDataHead;
			subDataDescriptor = reinterpret_cast<SubTextureDescriptor*>(image.data.data);

			this->gpuDataToken = UUID::invalid;
			return *this;
		}

		//we make a copy of the original image if we use normal renference
		Texture(const Texture& image) {
			data = image.data;
			width = image.width, height = image.height;
			rowPitch = image.rowPitch;
			type = image.type;
			subDataHead = image.subDataHead;
			subDataDescriptor = reinterpret_cast<SubTextureDescriptor*>(image.data.data);

			this->gpuDataToken = UUID::invalid;
		}

		Texture():data(),width(0),height(0),rowPitch(0),type(TEXTURE_TYPE::INVAILD),gpuDataToken(),
			subDataHead({0,0,0}),subDataDescriptor(nullptr) {}
	};

	class TextureLoader {
	public:
		//load image data from file,decompress data and generate a RBGA image buffer for user
		//how to decompress the image data determined by the extension name of the image file
		Texture loadImage(const char* filename);

		//load cude map from 6 image files,textures will be in the same order as the file array
		//the sample order of the array is +x,-x,+y,-y,+z,-z
		//the textures in the cube map must have the same height and the same width
		Texture loadCubeMap(CubeMapFileArray fileArray);
		//Texture loadCubeMap();
	private:
		enum IMAGE_TYPE {
			BMP,INVAILD
		};

		Texture bmpLoader(Buffer& data);

		inline uint32_t inverseEndian(uint32_t num) {
			return (num & 0xff) << 24 | (num & 0xff00) << 8 | (num & 0xff0000) >> 8 | (num & 0xff000000) >> 24;
		}
	};
}