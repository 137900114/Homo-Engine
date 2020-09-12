#pragma once
#include "Memory.h"

//a simple reference count based garbage collecting system 
namespace Game{

	extern MemoryModule* gMemory;
	//currently we use normal int 32 as Atom32UInt
	//because our program only has one thread
	using Atom32Int = int32_t;
	inline uint32_t add_atom(Atom32Int& target,int32_t num) {
		target += num;
		return target;
	}

	inline int32_t get_value_atom(Atom32Int& target) {
		return target;
	}

	//any object can be managed by CountPtr should be devired from CountedReference
	class CountedRefernce {
	public:
		CountedRefernce():count(0) {}
		virtual ~CountedRefernce() {}

		inline int32_t add_reference(int32_t num) {
			return add_atom(count,num);
		}

		inline int32_t get_reference_count() {
			return get_value_atom(count);
		}
	private:
		Atom32Int count;
	};

	template<typename T>
	class CountPtr {
	public:
		CountPtr(T* other) {
			if (ref && ref->add_reference(-1) <= 0) {
				gMemory->Delete(ref);
			}
			ref = other;
			other->add_reference(1);
		}
		CountPtr():ref(nullptr) {}
		CountPtr(const CountPtr& other) {
			if (ref && ref->add_reference(-1) <= 0) {
				gMemory->Delete(ref);
			}
			ref = const_cast<T*>(other.ref);
			ref->add_reference(1);
		}
		CountPtr(CountPtr&& other) {
			std::swap(ref,other.ref);
		}
		
		CountPtr& operator=(T* other) {
			if (ref && ref->add_reference(-1) <= 0) {
				gMemory->Delete(ref);
			}
			ref = other;
			other->add_reference(1);
		}
		CountPtr& operator=(const CountPtr& other) {
			if (ref && ref->add_reference(-1) <= 0) {
				gMemory->Delete(ref);
			}
			ref = const_cast<T*>(other);
			ref->add_reference(1);
		}
		CountPtr& operator=(CountPtr&& other) {
			std::swap(ref,other.ref);
		}
		
		~CountPtr() {
			if (ref && ref->add_reference(-1) <= 0) {
				gMemory->Delete(ref);
			}
		}

		T* operator->() { return ref; }
		T* get() { return ref; }
		T& operator*() { return *ref; }
	private:
		T* ref = nullptr;
	};

	template<typename T,typename ...Args>
	CountPtr<T> make_cptr(Args... args) {
		return std::move( CountPtr<T>(gMemory->New(args...)));
	}
}