#include "Image.h"
#include "FileLoader.h"
#include "Common.h"

namespace Game {
	extern FileLoader* gFileLoader;
}



Game::Texture Game::TextureLoader::loadImage(const char* filename) {
	auto get_file_extention = [](const char* filename)->IMAGE_TYPE {
		int i = 0;
		for (; filename[i] != '.'; i++) {
			if (filename[i] == '\0')  return IMAGE_TYPE::INVAILD;
		}
		if (strcmp(&filename[++i], "bmp") == 0) {
			return IMAGE_TYPE::BMP;
		}
		else
			return IMAGE_TYPE::INVAILD;
	};

	IMAGE_TYPE type = get_file_extention(filename);
	if (type == IMAGE_TYPE::INVAILD) {
		Log("image loader : image loader doesn't support file %s\n",filename);
		return std::move(Texture());
	}

	FileLoader::FilePtr file;
	if (!gFileLoader->Exists(filename,&file)) {
		Log("image loader : the file %s doesn't exists in search path!\n",filename);
		return std::move(Texture());
	}

	Buffer fileData;
	gFileLoader->FileRead(file,fileData);
	gFileLoader->FileClose(file);

	switch (type) {
	case BMP:
		return std::move(bmpLoader(fileData));
	}

	Log("image loader : unknown error occurs while loading image %s!\n");
	return std::move(Texture());
}


Game::Texture Game::TextureLoader::loadCubeMap(Game::CubeMapFileArray fileArray) {
	Texture cubeMapTex[6];
	size_t dataSize = 0;
	uint32_t width, height;

	//currently we assume that the texture loaded only have 1 mipmap level
	cubeMapTex[0] = std::move(loadImage(fileArray.right));
	if (cubeMapTex[0].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[0].subDataHead.subResourceNum != 1)
		return std::move(Texture());
	else{
		dataSize += cubeMapTex[0].data.size;
		width = cubeMapTex[0].width;
		height = cubeMapTex[0].height;
	}

	cubeMapTex[1] = std::move(loadImage(fileArray.left));
	if (cubeMapTex[1].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[1].subDataHead.subResourceNum != 1||
		 cubeMapTex[1].width != width || cubeMapTex[1].height != height)
		return std::move(Texture());
	else {
		dataSize += cubeMapTex[1].data.size;
	}

	cubeMapTex[2] = std::move(loadImage(fileArray.up));
	if (cubeMapTex[2].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[2].subDataHead.subResourceNum != 1 ||
		cubeMapTex[2].width != width || cubeMapTex[2].height != height)
		return std::move(Texture());
	else {
		dataSize += cubeMapTex[2].data.size;
	}

	cubeMapTex[3] = std::move(loadImage(fileArray.down));
	if (cubeMapTex[3].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[3].subDataHead.subResourceNum != 1 ||
		cubeMapTex[3].width != width || cubeMapTex[3].height != height)
		return std::move(Texture());
	else {
		dataSize += cubeMapTex[3].data.size;
	}

	cubeMapTex[4] = std::move(loadImage(fileArray.front));
	if (cubeMapTex[4].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[4].subDataHead.subResourceNum != 1 ||
		cubeMapTex[4].width != width || cubeMapTex[4].height != height)
		return std::move(Texture());
	else {
		dataSize += cubeMapTex[4].data.size;
	}

	cubeMapTex[5] = std::move(loadImage(fileArray.back));
	if (cubeMapTex[5].type != TEXTURE_TYPE::TEXTURE2D || cubeMapTex[5].subDataHead.subResourceNum != 1 ||
		cubeMapTex[5].width != width || cubeMapTex[5].height != height)
		return std::move(Texture());
	else {
		dataSize += cubeMapTex[5].data.size;
	}

	Texture result;
	result.type = TEXTURE_TYPE::CUBE;
	result.width = width;
	result.height = height;
	result.data.resize(dataSize + 6 * sizeof(SubTextureDescriptor));
	result.subDataHead.arrayNum = 6;
	result.subDataHead.mipMapNum = 1;
	result.subDataHead.subResourceNum = 6;
	result.gpuDataToken = UUID::invalid;

	result.subDataDescriptor = reinterpret_cast<SubTextureDescriptor*>(result.data.data);
	uint8_t* currDataPtr = reinterpret_cast<uint8_t*>(result.data.data) + 
				sizeof(SubTextureDescriptor) * 6;

	for (int i = 0; i != 6; i++) {
		result.subDataDescriptor[i].resourceData = currDataPtr;
		result.subDataDescriptor[i].rowPitch = cubeMapTex[i].rowPitch;
		result.subDataDescriptor[i].slicePitch = cubeMapTex[i].data.size;

		memcpy(currDataPtr,cubeMapTex[i].data.data,cubeMapTex[i].data.size);
		currDataPtr += cubeMapTex[i].data.size;
	}

	return std::move(result);
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

Game::Texture Game::TextureLoader::bmpLoader(Buffer& buf) {
	BMP_FILE_HEADER* file_header = reinterpret_cast<BMP_FILE_HEADER*>(buf.data);
	BMP_HEADER* header = reinterpret_cast<BMP_HEADER*>(buf.data + sizeof(BMP_FILE_HEADER));
	Texture result;
	if (file_header->bmpType == 0x4d42) {
		result.width = header->imageWidth,result.height = header->imageHeight;
		uint32_t pitch_size = ((header->imageBitCount * result.width + 31) >> 5 ) << 2; 

		auto check_validity = [&]()->bool {
			return header->compression == 0x0 && header->clrUsed == 0 && header->clrImportant == 0
				&& header->imageBitCount == 24;
		};

		if (check_validity()) {
			//create a rgba image
			result.data.resize(result.width * result.height * 4);
			result.rowPitch = result.width * 4;
			//the image data is 4 byte alisned
			uint8_t* image_data = buf.data + file_header->bmpOffbits;
			for (int y = 0; y != result.height;y++) {
				for (int x = 0; x != result.width;x++) {
					uint8_t* currPos = result.data.data + x * 4 + y * (result.width * 4);
					//We should load data in opposite diection in y axis
					uint8_t* bufPos = image_data + x * (header->imageBitCount >> 3) + y * pitch_size;
					//The color order in file is BGR
					currPos[0] = bufPos[2];
					currPos[1] = bufPos[1];
					currPos[2] = bufPos[0];
					currPos[3] = 0xff;
				}
			}

			//for a single texture we don't have to 
			result.type = TEXTURE_TYPE::TEXTURE2D;
			result.subDataHead.arrayNum = 1;
			result.subDataHead.mipMapNum = 1;
			result.subDataHead.subResourceNum = 1;

			result.subDataDescriptor = nullptr;

			return std::move(result);
		}
	}

	Log("image loader : we don't support this type of bmp header\n");
	return std::move(result);
	
}