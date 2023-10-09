#pragma once
/**
 * 'Frieren' assumes that type of 'std::string' instances must be a 'UTF-8 encoded string'.
 * 'Frieren' assumes that source code files are encoded in UTF-8.
 * Also, All strings stored in memory are instances of std::string.
 * When string comes in either UTF-16 encoded or as a wide characters. It must converted to UTF-8 encoded string using 'Narrower' function.
 * Basically, 'Frieren' is targeting the 'Windows Platform'. Sometimes, It should pass string as 'Wider Character String'. In such cases, you must use 'Wider' functions to convert UTF-8 encoded string to wider character string.
 * For all literal constant strings, they must be used with the 'FE_TEXT macro function'.
 */
#include <string>
#ifndef UTF_CPP_CPLUSPLUS 
#define UTF_CPP_CPLUSPLUS 201703L
#endif
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

	inline bool IsValidUTF8(const std::string_view string)
	{
		return utf8::is_valid(string);
	}
}