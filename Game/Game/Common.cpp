#include "Common.h"
#include <sstream>


void str_split(std::string str,std::vector<std::string>& target,char dim) {
	std::istringstream iss(str);
	std::string temp;

	target.clear();
	while (std::getline(iss,temp,dim)) {
		target.push_back(temp);
	}
}


void getExtensionName(const char* filename, std::string& file_extension, std::string& file_name) {
	std::string _filename(filename);
	size_t ext_pos = _filename.find_last_of('.');
	file_extension = _filename.substr(ext_pos + 1, _filename.size() - ext_pos - 1),
		file_name = _filename.substr(0,ext_pos);
}

void printBuffer(const void* data,size_t size) {
	const uint8_t* _data = reinterpret_cast<const uint8_t*>(data);

	for (int i = 0; i != 16; i++) {
		printf("%4x",i);
	}

	int j = 0;
	for (int i = 0; i != size;i++) {
		if (j % 16 == 0) {
			printf("\n");
		}
		printf("%4X",*(_data + i));
		j++;
	}
}