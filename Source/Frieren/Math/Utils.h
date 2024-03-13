#pragma once
#include <Math/Common.h>

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

	inline constexpr float Pi32 = 3.14159265f;
	inline constexpr float InversePi32 = 1.f / Pi32;

	inline constexpr float Rad2Deg(const float radians)
	{
		constexpr float rad2deg = Pi32 / 180.f;
		return radians * rad2deg;
	}

	inline constexpr float Deg2Rad(const float degrees)
	{
		constexpr float deg2rad = 180.f / Pi32;
		return degrees * deg2rad;
	}
} // namespace fe