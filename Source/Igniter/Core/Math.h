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
    consteval T Pow(const T base, const size_t exp)
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

    inline constexpr float Pi32        = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;
    inline constexpr float InversePi32 = 1.f / Pi32;

    inline constexpr float Rad2Deg(const float radians)
    {
        constexpr float rad2deg = 180.f / Pi32;
        return radians * rad2deg;
    }

    inline constexpr float Deg2Rad(const float degrees)
    {
        constexpr float deg2rad = Pi32 / 180.f;
        return degrees * deg2rad;
    }

    template <typename T>
    constexpr T Lerp(const T a, const T b, const float t)
    {
        return (1 - t) * a + t * b;
    }

    constexpr float SmoothStep(float min, float max, const float x)
    {
        const float t = std::clamp<float>((x - min) / (max - min), 0.0f, 1.0f);
        return t * t * (3.f - 2.f * t);
    }

    constexpr float SmootherStep(float min, float max, const float x)
    {
        const float t = std::clamp<float>((x - min) / (max - min), 0.0f, 1.0f);
        return t * t * t * (t * (6.f * t - 15.f) + 10.f);
    }
} // namespace ig
