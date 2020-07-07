#include "Image.h"
#include "FileLoader.h"
#include "Common.h"

namespace Game {
	extern FileLoader* gFileLoader;
}



Game::Image&& Game::ImageLoader::loadImage(const char* filename) {
	auto get_file_extention = [](const char* filename)->IMAGE_TYPE {
		int i = 0;
		for (; filename[i] != '.'; i++) {
			if (filename[i] == '\0')  return IMAGE_TYPE::INVAILD;
		}
		if (strcmp(&filename[++i], "bmp")) {
			return IMAGE_TYPE::BMP;
		}
		else
			return IMAGE_TYPE::INVAILD;
	};

	IMAGE_TYPE type = get_file_extention(filename);
	if (type == IMAGE_TYPE::INVAILD) {
		Log("image loader : image loader doesn't support file %s\n",filename);
		return std::move(Image());
	}

	FileLoader::FilePtr file;
	if (!gFileLoader->Exists(filename,&file)) {
		Log("image loader : the file %s doesn't exists in search path!\n",filename);
		return std::move(Image());
	}

	Buffer fileData;
	gFileLoader->FileRead(file,fileData);
	gFileLoader->FileClose(file);

	switch (type) {
	case BMP:
		return std::move(bmpLoader(fileData));
	}

	Log("image loader : unknown error occurs while loading image %s!\n");
	return std::move(Image());
}

//The header should not be alisned in memory or we will access wrong data
#pragma pack(push,1)
struct BMP_FILE_HEADER{
	uint16_t bmpType;
	uint32_t fileSize;
	uint32_t bmpReserved;
	uint32_t bmpOffbits;
};

struct BMP_HEADER {
	uint32_t headerSize;
	uint32_t imageWidth;
	uint32_t imageHeight;
	uint16_t imagePlanes;
	uint16_t imageBitCount;
	uint32_t compression;
	uint32_t imageSize;
	uint32_t imageResolution[2];
	uint32_t clrUsed;
	uint32_t clrImportant;
};

#pragma pack(pop)

Game::Image&& Game::ImageLoader::bmpLoader(Buffer& buf) {
	BMP_FILE_HEADER* file_header = reinterpret_cast<BMP_FILE_HEADER*>(buf.data);
	BMP_HEADER* header = reinterpret_cast<BMP_HEADER*>(buf.data + sizeof(BMP_FILE_HEADER));
	Image result;
	if (file_header->bmpType == 0x4d42) {
		result.width = header->imageWidth,result.height = header->imageHeight;
		uint32_t pitch_size = (header->imageBitCount * result.width + 3) & (~3); 

		auto check_validity = [&]()->bool {
			return header->compression == 0x0 && header->clrUsed == 0 && header->clrImportant == 0
				&& header->imageBitCount == 24;
		};

		if (check_validity()) {
			//create a rgba image
			result.data.resize(result.width * result.height * 4);

			//the image data is 4 byte alisned
			uint8_t* image_data = buf.data + file_header->fileSize;
			for (int y = 0; y != result.height;y++) {
				for (int x = 0; x != result.width;x++) {
					uint8_t* currPos = result.data.data + x * 4 + y * (result.width * 4);
					//We should load data in opposite diection in y axis
					uint8_t* bufPos = image_data + x * (header->imageBitCount >> 3) + (result.height - 1 - y) * pitch_size;
					//The color order in file is BGR
					currPos[0] = bufPos[2];
					currPos[1] = bufPos[1];
					currPos[2] = bufPos[0];
					currPos[3] = 0xff;
				}
			}

			return std::move(result);
		}
	}

	Log("image loader : we don't support this type of bmp header\n");
	return std::move(result);
	
}