#pragma once
#include <locale>
#include <codecvt>

namespace Utils {

	
	inline std::wstring to_wide_str(const std::string& input) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(input);
	}

	inline std::string to_byte_str(const std::wstring& input) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(input);
	}
	
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