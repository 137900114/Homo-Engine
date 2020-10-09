#pragma once
#include "Memory.h"
namespace Game {
	extern MemoryModule gMemory;

	struct Buffer {
		Buffer() :size(0), data(nullptr) {}
		Buffer(size_t size) :size(size)
		{
			if (size != 0) {
				data = reinterpret_cast<uint8_t*>(gMemory.allocate(size));
			}
			else {
				data = nullptr;
			}
		}
		~Buffer() { release(); }

		void release() { if (data) gMemory.deallocate(size, data); data = nullptr, size = 0; }
		void resize(size_t size) {
			release();
			data = reinterpret_cast<uint8_t*>(gMemory.allocate(size));
			this->size = size;
		}

		Buffer& operator=(const Buffer& buf){
			resize(buf.size);
			memcpy(data,buf.data,size);
			return *this;
		}

		Buffer(const Buffer& buf) {
			data = reinterpret_cast<uint8_t*>(gMemory.allocate(buf.size));
			memcpy(data,buf.data,buf.size);
			size = buf.size;
		}

		//the original buffer will be invalid after the move operation
		Buffer& operator=(Buffer&& buf) noexcept {
			release();
			data = buf.data;
			size = buf.size;
			buf.data = nullptr;
			buf.size = 0;
			return *this;
		}

		Buffer(Buffer&& buf) noexcept {
			data = buf.data;
			size = buf.size;
			buf.data = nullptr;
			buf.size = 0;
		}

		uint8_t* data;
		size_t size;
	};
}