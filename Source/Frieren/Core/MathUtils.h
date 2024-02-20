#pragma once
#include <concepts>
#include <cmath>
#include <random>

namespace fe
{
	template <typename T>
		requires std::unsigned_integral<T>
	constexpr bool IsPowOf2(T val)
	{
		return val == 0 ? false : (val & (val - 1)) == 0;
	}

	template <typename T>
		requires std::integral<T>
	T Random(const T min, const T max = std::numeric_limits<T>::max())
	{
		static thread_local std::mt19937_64 generator;
		return std::uniform_int_distribution<T>{ min, max }(generator);
	}
} // namespace fe