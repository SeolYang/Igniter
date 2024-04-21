#pragma once
#include <Igniter.h>
#include <Core/Math.h>

namespace ig
{
    constexpr size_t CalculateAlignAdjustment(const size_t size, const size_t alignment)
    {
        IG_CHECK(size >= sizeof(void*));
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const size_t mask         = alignment - 1;
        const size_t misalignment = size & mask;
        return (alignment - misalignment);
    }

    inline uint64_t AlignTo(const uint64_t val, const uint64_t alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        return ((val + alignment - 1) / alignment) * alignment;
    }

    template <typename T>
    constexpr T AlignUp(T value, const size_t alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const size_t mask = alignment - 1;
        return (T)((size_t)(value + mask) & ~mask);
    }

    template <typename T>
    T AlignDown(T value, const size_t alignment)
    {
        IG_CHECK(alignment > 0 && IsPowOf2(alignment));
        const size_t mask = alignment - 1;
        return (T)((size_t)value & ~mask);
    }

    template <size_t OffsetInBits, size_t SizeInBits, typename Ty = uint64_t>
    constexpr Ty MaskBits(const uint64_t value)
    {
        static_assert(SizeInBits <= sizeof(Ty) * 8);
        static_assert(SizeInBits + OffsetInBits <= sizeof(uint64_t) * 8);
        return (value >> OffsetInBits) & ((1Ui64 << SizeInBits) - 1);
    }

    template <size_t OffsetInBits, size_t SizeInBits>
    constexpr uint64_t SetBits(const uint64_t dest, const uint64_t src)
    {
        static_assert(SizeInBits + OffsetInBits <= sizeof(uint64_t) * 8);
        return dest | ((((1Ui64 << SizeInBits) - 1) & src) << OffsetInBits);
    }

    inline constexpr double BytesToKiloBytes(const size_t bytes)
    {
        return bytes / 1024.0;
    }

    inline constexpr double BytesToMegaBytes(const size_t bytes)
    {
        return bytes / (1024.0 * 1024.0);
    }

    inline constexpr double BytesToGigaBytes(const size_t bytes)
    {
        return bytes / (1024.0 * 1024.0 * 1024.0);
    }
} // namespace ig
