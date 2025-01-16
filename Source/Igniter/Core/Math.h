#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    template <typename T>
        requires std::unsigned_integral<T>
    constexpr bool IsPowOf2(T val)
    {
        return val == 0 ? false : (val & (val - 1)) == 0;
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    consteval T Pow(const T base, const Size exp)
    {
        if (exp == 0)
        {
            return T{1};
        }

        T result = base;
        for ([[maybe_unused]] const auto _ : views::iota(0Ui64, exp - 1))
        {
            result *= base;
        }
        return result;
    }

    template <typename T>
        requires std::integral<T>
    T Random(const T min, const T max = std::numeric_limits<T>::max())
    {
        static thread_local std::mt19937_64 generator;
        return std::uniform_int_distribution<T>{min, max}(generator);
    }

    template <typename T>
        requires std::floating_point<T>
    T Random(const T min, const T max)
    {
        static thread_local std::mt19937_64 generator;
        return std::uniform_real_distribution<T>{min, max}(generator);
    }

    inline constexpr F32 Pi32 = std::numbers::pi_v<F32>;
    inline constexpr F32 InversePi32 = 1.f / Pi32;

    inline constexpr F32 Rad2Deg(const F32 radians)
    {
        constexpr F32 rad2deg = 180.f / Pi32;
        return radians * rad2deg;
    }

    inline constexpr F32 Deg2Rad(const F32 degrees)
    {
        constexpr F32 deg2rad = Pi32 / 180.f;
        return degrees * deg2rad;
    }

    template <typename T>
    constexpr T Lerp(const T a, const T b, const F32 t)
    {
        return (1 - t) * a + t * b;
    }

    constexpr F32 SmoothStep(F32 min, F32 max, const F32 x)
    {
        const F32 t = std::clamp<F32>((x - min) / (max - min), 0.0f, 1.0f);
        return t * t * (3.f - 2.f * t);
    }

    constexpr F32 SmootherStep(F32 min, F32 max, const F32 x)
    {
        const F32 t = std::clamp<F32>((x - min) / (max - min), 0.0f, 1.0f);
        return t * t * t * (t * (6.f * t - 15.f) + 10.f);
    }
} // namespace ig
