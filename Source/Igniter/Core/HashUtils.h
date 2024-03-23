#pragma once
#include <Igniter.h>
#include <Core/MemUtils.h>

namespace ig
{
    inline constexpr uint64_t InvalidHashVal = 0xffffffffffffffffUi64;

    inline uint64_t HashRange(const uint32_t* const begin, const uint32_t* const end, uint64_t hash)
    {
#if ENABLE_SSE_CRC32
        const uint64_t* iter64 = (const uint64_t*)AlignUp(begin, 8);
        const uint64_t* const end64 = (const uint64_t* const)AlignDown(end, 8);

        if ((uint32_t*)iter64 > begin)
        {
            hash = _mm_crc32_u32((uint32_t)hash, *begin);
        }

        while (iter64 < end64)
        {
            hash = _mm_crc32_u64((uint64_t)hash, *iter64++);
        }

        if ((uint32_t*)iter64 < end)
        {
            hash = _mm_crc32_u32((uint32_t)hash, *(uint32_t*)iter64);
        }
#else
        for (const uint32_t* iter = begin; iter < end; ++iter)
        {
            hash = 16777619U * hash ^ *iter;
        }
#endif

        return hash;
    }

    /* #sy_ref DirectX-Graphics-Samples */
    template <typename T>
    inline uint64_t HashState(const T* stateDesc, size_t count = 1, uint64_t hash = 2166136261U)
    {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
        return HashRange((uint32_t*)stateDesc, (uint32_t*)(stateDesc + count), hash);
    }

    constexpr uint64_t HashStringSimple(const std::string_view targetString)
    {
        uint64_t result = 0;

        for (const auto& c : targetString)
        {
            (result ^= c) <<= 1;
        }

        return result;
    }

    /**
     * #sy_ref	https://github.com/srned/baselib/blob/master/crc64.c
     */
    constexpr uint64_t EvalCRC64(const std::string_view data)
    {
        uint64_t result = 0;
        for (const unsigned char byte : data)
        {
            result = table::CRC64[(uint8_t)result ^ byte] ^ (result >> 8);
        }

        return result;
    }

    /**
     * #sy_ref https://stackoverflow.com/questions/56292104/hashing-types-at-compile-time-in-c17-c2a
     */
    template <typename T>
    constexpr uint64_t EvalHashOfType()
    {
        return EvalCRC64(__FUNCSIG__);
    }

    template <typename T>
    constexpr uint64_t HashOfType = EvalHashOfType<std::decay_t<T>>();
} // namespace ig