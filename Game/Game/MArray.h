#pragma once
#include "Buffer.h"
#include <new.h>

namespace Game {
	
	template<typename T>
	class MArray {
	public:
		template<typename ...Args>
		MArray(uint32_t num,Args ...args) {
			data.resize(sizeof(T) * num);
			object_num = num;
			for (int i = 0; i != num; i++) {
				void* object_base = i * sizeof(T) + data.data;
				new (object_base) T(args);
			}
		}

		MArray(): data(0),object_num(0) {}

		MArray(uint32_t num) {
			data.resize(sizeof(T) * num);
			object_num = num;
			for (int i = 0; i != num; i++) {
				void* object_base = i * sizeof(T) + data.data;
				new (object_base) T();
			}
		}


		T& operator[](uint32_t index) {
			T* object = reinterpret_cast<T*>(data.data + index * sizeof(T));
			return *object;
		}

		const T& operator[](uint32_t index) const {
			const T* object = reinterpret_cast<const T*>(data.data + index * sizeof(T));
			return *object;
		}

		inline uint32_t size() { return object_num; }

		template<typename ...Args>
		void resize(uint32_t newnum,Args ...args) {
			Buffer temp(newnum * sizeof(T));
			memcpy(temp.data,data.data,newnum < object_num ? temp.size : data.size);

			for (int i = object_num; i < newnum; i++) {
				void* object_base = i * sizeof(T) + temp.data;
				new (object_base) T(args);
			}
			std::swap(data,temp);
			object_num = newnum;
		}

		void resize(uint32_t newnum) {
			Buffer temp(newnum * sizeof(T));
			memcpy(temp.data,data.data,newnum < object_num ? temp.size : data.size);

			for (int i = object_num; i < newnum; i++) {
				void* object_base = i * sizeof(T) + temp.data;
				new (object_base) T();
			}
			object_num = newnum;
			std::swap(data,temp);
		}


	private:
		Buffer data;
		uint32_t object_num;
	};
}