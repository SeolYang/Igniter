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
}

#define NUMERIC_MIN_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::min()
#define NUMERIC_MAX_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::max()
