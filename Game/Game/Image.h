#pragma once
#include "Buffer.h"
#include "uuid.h"

namespace Game {
	//Image is a buffer type object which means that 
	//outside should use pointer reference it rather than making copy of it
	//because the copy operation will make the original buffer invaild
	//
	struct Image{
		size_t width, height;
		Buffer data;
		//you can use a uuid to represent data uploaded to gpu
		UUID gpuDataToken;

		Image& operator=(Image&& image) noexcept {
			data = std::move(image.data);
			width = image.width, height = image.height;
			image.width = 0, image.height = 0;
			
			this->gpuDataToken = std::move(image.gpuDataToken);
			return *this;
		}

		Image(Image&& image) noexcept {
			data = std::move(image.data);
			width = image.width, height = image.height;
			image.width = 0, image.height = 0;

			this->gpuDataToken = std::move(image.gpuDataToken);
		}

		//we make a copy of the original image if we use normal renference 
		Image& operator=(const Image& image) {
			data = image.data;
			width = image.width, height = image.height;

			this->gpuDataToken = image.gpuDataToken;
			return *this;
		}

		//we make a copy of the original image if we use normal renference
		Image(const Image& image) {
			data = image.data;
			width = image.width, height = image.height;

			this->gpuDataToken = image.gpuDataToken;
		}

		Image():data(),width(0),height(0),gpuDataToken(){}
	};

	class ImageLoader {
	public:
		//load image data from file,decompress data and generate a RBGA image buffer for user
		//how to decompress the image data determined by the extension name of the image file
		Image&& loadImage(const char* filename);
	private:
		enum IMAGE_TYPE {
			BMP,INVAILD
		};

		Image&& bmpLoader(Buffer& data);

		inline uint32_t inverseEndian(uint32_t num) {
			return (num & 0xff) << 24 | (num & 0xff00) << 8 | (num & 0xff0000) >> 8 | (num & 0xff000000) >> 24;
		}
	};
}