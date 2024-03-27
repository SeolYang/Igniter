#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename T>
        requires std::is_arithmetic_v<std::decay_t<T>>
    consteval T NumericMinOfValue(const T&)
    {
        return std::numeric_limits<std::decay_t<T>>::min();
    }

    template <typename T>
        requires std::is_arithmetic_v<std::decay_t<T>>
    consteval T NumericMaxOfValue(const T&)
    {
        return std::numeric_limits<std::decay_t<T>>::max();
    }

    template <typename T, typename F>
    constexpr bool ContainsFlags(const T& flags, const F& flag)
    {
        const auto bits = static_cast<F>(flags);
        return ((bits & flag) == flag);
    }
} // namespace ig

#define IG_NUMERIC_MIN_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::min()
#define IG_NUMERIC_MAX_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::max()

#define IG_ENUM_FLAGS(ENUM_TYPE)                                                            \
    inline constexpr ENUM_TYPE operator|(const ENUM_TYPE lhs, const ENUM_TYPE rhs)          \
    {                                                                                       \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                           \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) | \
                                      static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }                                                                                       \
    inline constexpr ENUM_TYPE operator&(const ENUM_TYPE lhs, const ENUM_TYPE rhs)          \
    {                                                                                       \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                           \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) & \
                                      static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }