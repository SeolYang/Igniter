#pragma once
#include <concepts>

namespace fe
{
	template <typename T>
		requires std::unsigned_integral<T>
	constexpr bool IsPowOf2(T val)
	{
		return val == 0 ? false : (val & (val - 1)) == 0;
	}
}