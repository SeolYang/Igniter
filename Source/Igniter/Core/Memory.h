#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Math.h"

namespace ig
{
    constexpr Size CalculateAlignAdjustment(const Size size, const Size alignment)
    {
        IG_CHECK(size >= sizeof(void*));
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const Size mask = alignment - 1;
        const Size misalignment = size & mask;
        return (alignment - misalignment);
    }

    inline uint64_t AlignTo(const uint64_t val, const uint64_t alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        return ((val + alignment - 1) / alignment) * alignment;
    }

    template <typename T>
    constexpr T AlignUp(T value, const Size alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const Size mask = alignment - 1;
        return (T)((Size)(value + mask) & ~mask);
    }

    template <typename T>
    T AlignDown(T value, const Size alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const Size mask = alignment - 1;
        return (T)((Size)value & ~mask);
    }

    template <Size OffsetInBits, Size SizeInBits, typename Ty = uint64_t>
    constexpr Ty MaskBits(const uint64_t value)
    {
        static_assert(SizeInBits <= sizeof(Ty) * 8);
        static_assert(SizeInBits + OffsetInBits <= sizeof(uint64_t) * 8);
        return (value >> OffsetInBits) & ((1Ui64 << SizeInBits) - 1);
    }

    template <Size OffsetInBits, Size SizeInBits>
    constexpr uint64_t SetBits(const uint64_t dest, const uint64_t src)
    {
        static_assert(SizeInBits + OffsetInBits <= sizeof(uint64_t) * 8);
        return dest | ((((1Ui64 << SizeInBits) - 1) & src) << OffsetInBits);
    }

    inline constexpr Size BytesToBits(const Size bytes)
    {
        return bytes * 8Ui64;
    }

    inline constexpr double BytesToKiloBytes(const Size bytes)
    {
        return bytes / 1024.0;
    }

    inline constexpr double BytesToMegaBytes(const Size bytes)
    {
        return bytes / (1024.0 * 1024.0);
    }

    inline constexpr double BytesToGigaBytes(const Size bytes)
    {
        return bytes / (1024.0 * 1024.0 * 1024.0);
    }
} // namespace ig
