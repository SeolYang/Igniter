#pragma once
#include <numeric>
#include <concepts>

namespace fe
{
	template <typename T>
		requires std::is_arithmetic_v<std::decay_t<T>>
	consteval T NumericMinOfValue(const T&)
	{
		return std::numeric_limits<std::decay_t<T>>::min();
	}

	template <typename T>
		requires std::is_arithmetic_v<std::decay_t<T>>
	constexpr T NumericMaxOfValue(const T&)
	{
		return std::numeric_limits<std::decay_t<T>>::max();
	}

	template <typename T>
	constexpr bool BitFlagContains(const T& bits, const T& flag)
	{
		return ((bits & flag) == flag);
	}

	template <typename T, typename... Args>
	constexpr bool BitFlagContains(const T& bits, const T& flag, const Args&... args)
	{
		return ((bits & flag) == flag) && BitFlagContains(bits, args...);
	}
} // namespace fe

#define NUMERIC_MIN_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::min()
#define NUMERIC_MAX_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::max()