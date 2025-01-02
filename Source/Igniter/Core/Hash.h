#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Memory.h"

namespace ig
{
    inline constexpr uint64_t InvalidHashVal = 0xffffffffffffffffUi64;

    template <typename Ty>
    inline U32 HashInstance(const Ty& instance)
    {
        constexpr Size NumDwordChunks       = sizeof(Ty) / 4u;
        constexpr Size NumRemainingBytes    = sizeof(Ty) % 4u;
        constexpr Size RemainingBytesOffset = sizeof(Ty) - NumRemainingBytes;

        const U32* const instanceDwordsPtr = reinterpret_cast<const U32*>(&instance);
        const U8* const  instanceBytesPtr  = reinterpret_cast<const U8*>(&instance) + RemainingBytesOffset;

#if ENABLE_SSE_CRC32
        U32 hash = 0xFFFFFFFFu;
        for (Index dwordChunkIdx = 0; dwordChunkIdx < NumDwordChunks; ++dwordChunkIdx)
        {
            hash = _mm_crc32_u32(hash, instanceDwordsPtr[dwordChunkIdx]);
        }

        for (Index byteIdx = 0; byteIdx < NumRemainingBytes; ++byteIdx)
        {
            hash = _mm_crc32_u32(hash, instanceBytesPtr[byteIdx]);
        }
#else
        constexpr U32 FnvPrime       = 16777619u;
        constexpr U32 FnvOffsetBasis = 2166136261u;

        U32 hash = FnvOffsetBasis;
        for (Index dwordChunkIdx = 0; dwordChunkIdx < NumDwordChunks; ++dwordChunkIdx)
        {
            hash = FnvPrime * hash ^ instanceDwordsPtr[dwordChunkIdx];
        }

        for (Index byteIdx = 0; byteIdx < NumRemainingBytes; ++byteIdx)
        {
            hash = FnvPrime * hash ^ instanceBytesPtr[byteIdx];
        }
#endif
        return hash ^ 0xFFFFFFFFu;
    }

    inline uint64_t HashRange(const uint32_t* const begin, const uint32_t* const end, uint64_t hash)
    {
#if ENABLE_SSE_CRC32
        const uint64_t*       iter64 = (const uint64_t*)AlignUp(begin, 8);
        const uint64_t* const end64  = (const uint64_t* const)AlignDown(end, 8);

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
    uint64_t HashState(const T* stateDesc, size_t count = 1, uint64_t hash = 2166136261U)
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
    constexpr uint32_t TypeHash = entt::type_hash<T>::value();

    /* #sy_ref https://stackoverflow.com/questions/3058139/hash-32bit-int-to-16bit-int */
    static constexpr uint16_t ReduceHashTo16Bits(const uint64_t hash)
    {
        return static_cast<uint16_t>((hash * 0x8000800080008001Ui64) >> 48);
    }
} // namespace ig
