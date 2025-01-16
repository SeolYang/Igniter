#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Memory.h"

namespace ig
{
    inline constexpr uint64_t InvalidHashVal = 0xffffffffffffffffUi64;

    template <typename Ty>
    inline U64 HashInstance(const Ty& instance)
    {
        static_assert(sizeof(Ty)%8 == 0);
        constexpr U64 FnvPrime = 16777619u;
        constexpr U64 FnvOffsetBasis = 2166136261u;
        constexpr Size NumChunks = sizeof(Ty) / 8;

        const U64* ptr = reinterpret_cast<const U64*>(&instance);
        U64 hash = FnvOffsetBasis;
        for (Index idx = 0; idx < NumChunks; ++idx)
        {
            hash = FnvPrime * hash ^ ptr[idx];
        }

        return hash ^ 0xFFFFFFFFFFFFFFFFllu;
    }

    inline uint64_t HashRange(const U32* const begin, const U32* const end, uint64_t hash)
    {
#if ENABLE_SSE_CRC32
        const uint64_t* iter64 = (const uint64_t*)AlignUp(begin, 8);
        const uint64_t* const end64 = (const uint64_t* const)AlignDown(end, 8);

        if ((U32*)iter64 > begin)
        {
            hash = _mm_crc32_u32((U32)hash, *begin);
        }

        while (iter64 < end64)
        {
            hash = _mm_crc32_u64((uint64_t)hash, *iter64++);
        }

        if ((U32*)iter64 < end)
        {
            hash = _mm_crc32_u32((U32)hash, *(U32*)iter64);
        }
#else
        for (const U32* iter = begin; iter < end; ++iter)
        {
            hash = 16777619U * hash ^ *iter;
        }
#endif

        return hash;
    }

    /* #sy_ref DirectX-Graphics-Samples */
    template <typename T>
    uint64_t HashState(const T* stateDesc, size_t count = 1, uint64_t hash = 2166136261U)
    {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
        return HashRange((U32*)stateDesc, (U32*)(stateDesc + count), hash);
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
    constexpr uint64_t EvalCRC64(const std::string_view data) noexcept
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
    constexpr uint64_t TypeHash64 = EvalCRC64(GetTypeName<T>());

    template <typename T>
    constexpr U32 TypeHash = entt::type_hash<T>::value();

    /* #sy_ref https://stackoverflow.com/questions/3058139/hash-32bit-int-to-16bit-int */
    static constexpr uint16_t ReduceHashTo16Bits(const uint64_t hash)
    {
        return static_cast<uint16_t>((hash * 0x8000800080008001Ui64) >> 48);
    }
} // namespace ig
