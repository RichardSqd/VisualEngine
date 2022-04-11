#pragma once

namespace Utils {

	inline void Print(const char* msg) { OutputDebugStringA(msg); }
	inline void Print(const wchar_t* msg) { OutputDebugString(msg); }

	#define STRINGIFY(x) #x
	#define TOSTRING(x) STRINGIFY(x)

	#define ASSERT(isFalse){     \
		if (!isFalse){		   \
			Utils::Print("\nASSERT failed in " __FILE__ " @ " TOSTRING(__LINE__) "\n");\
			__debugbreak();			\
		}                          \
	}


	#define BREAKIFFAILED(hr){     \
		if (FAILED(hr)){		   \
			Utils::Print("\nHRESULT failed in " __FILE__ " @ " TOSTRING(__LINE__) "\n");\
			__debugbreak();			\
		}                          \
	}	

	#define BREAKIFNULL(ptr){\
	if (ptr == nullptr) {\
		Utils::Print("\nNullptr found in " __FILE__ " @ " TOSTRING(__LINE__) "\n"); \
			__debugbreak();\
		}\
	}\
	
	#define PRINTERROR(){ \
	Utils::Print("\nError at " __FILE__ " @ " TOSTRING(__LINE__) "\n"); \
	__debugbreak();\
	}\

}