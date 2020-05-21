#pragma once
#include "common.h"

inline void output_warning(bool expression, const char* str) {
	if (!expression) {
#ifdef _DEBUG
		::OutputDebugStringA(str);
#endif
	}
}

inline void output_warning(bool expression, const wchar_t* str) {
	if (!expression) {
#ifdef _DEBUG
		::OutputDebugStringW(str);
#endif
	}
}

inline void fail_and_exit(bool expression,const char* str) {
	if (!expression) {
#ifdef _DEBUG
		::OutputDebugStringA(str);
#endif
		exit(-1);
	}
}

inline void fail_and_exit(bool expression,const wchar_t* str) {
	if (!expression) {
#ifdef _DEBUG
		::OutputDebugStringW(str);
#endif
		exit(-1);
	}
}

#define CONNECT_STR(x) #x

#ifdef _DEBUG
#define ASSERT_HR(hr,str)\
	if(FAILED(hr)){\
		::OutputDebugStringA("hr failed in " CONNECT_STR(__FILE__)"at" CONNECT_STR(__LINE__) "\n details:");\
		::OutputDebugStringA(str"\n");\
		__debugbreak();\
	}
#define ASSERT_WARNING(exp,str)\
	if(exp){\
		::OutputDebugStringA("hr failed in " CONNECT_STR(__FILE__)"at" CONNECT_STR(__LINE__) "\n details:");\
		::OutputDebugStringA(str"\n");\
		__debugbreak();\
	}
#else
#define ASSERT_HR(hr,str)
#define ASSERT_WARNING(exp,str)
#endif

#define DEBUG_OUTPUT ::OutputDebugStringA


class UnCopyable {
public:
	virtual ~UnCopyable() {}
	UnCopyable() {}
	UnCopyable(const UnCopyable&) = delete;
	void operator=(const UnCopyable&) = delete;
};