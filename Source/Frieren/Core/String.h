#pragma once
#ifndef UTF_CPP_CPLUSPLUS 
#define UTF_CPP_CPLUSPLUS 201703L
#endif

#include <string>
#include <utf8cpp/utf8.h>
#define FE_TEXT(x) (u8##x)

namespace fe
{
	inline std::wstring Wider(const std::string_view from)
	{
		std::wstring result;
		utf8::utf8to16(from.begin(), from.end(), std::back_inserter(result));
		return result;
	}

	inline std::string Narrower(const std::wstring_view from)
	{
		std::string result;
		utf8::utf16to8(from.begin(), from.end(), std::back_inserter(result));
		return result;
	}
}