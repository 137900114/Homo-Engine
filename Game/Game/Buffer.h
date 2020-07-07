#pragma once
#include "Memory.h"
namespace Game {
	extern MemoryModule* gMemory;

	struct Buffer {
		Buffer() :size(0), data(nullptr) {}
		Buffer(size_t size) :size(size),
			data(reinterpret_cast<uint8_t*>(gMemory->allocate(size)))
		{}
		~Buffer() { if (data) release(); }

		void release() { if (data) gMemory->deallocate(size, data); data = nullptr, size = 0; }
		void resize(size_t size) {
			if (data) release();
			data = reinterpret_cast<uint8_t*>(gMemory->allocate(size));
			this->size = size;
		}

		Buffer& operator=(const Buffer& buf){
			resize(buf.size);
			memcpy(data,buf.data,size);
			return *this;
		}

		Buffer(const Buffer& buf) {
			data = reinterpret_cast<uint8_t*>(gMemory->allocate(buf.size));
			memcpy(data,buf.data,buf.size);
			size = buf.size;
		}

		//the original buffer will be invaild after the move operation
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

	/*
	//ObjRef is not a smart pointer.We suggest only one copy of one ObjRef in the whole program
	template<typename T>
	struct ObjRef {

		template<typename ...Args>
		ObjRef(Args ...params):ptr(gMemory->New<T>(params)) {}
		
		~ObjRef() { gMemory->Delete(ptr); }

		T& operator*() { return *ptr; }
		const T& operator*() const { return *ptr; }

		ObjRef(const ObjRef& rhs) = delete;
		ObjRef& operator=(const ObjRef& rhs) = delete;

		T* ptr;
	};
	*/
}