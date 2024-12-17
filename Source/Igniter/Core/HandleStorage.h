#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Hash.h"
#include "Igniter/Core/Memory.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Core/Handle.h"
#ifdef IG_TRACK_LEAKED_HANDLE
#include "Igniter/Core/DebugTools.h"
#endif

namespace ig::details
{
    template <typename Ty>
    consteval size_t GetHeuristicOptimalChunkSize()
    {
        /* @TODO Fine-Tuning or find more optimal model */
        if (sizeof(Ty) < 512i64)
        {
            return 16Ui64 * 1024Ui64; /* 16 KB chunk for less 512 bytes alloc. */
        }

        return 4Ui64 * 1024Ui64 * 1024Ui64; /* 4 MB for greater 512 bytes alloc. */
    }
}    // namespace ig::details

namespace ig
{
    template <typename Ty, typename Dependency>
        requires(sizeof(Ty) >= sizeof(uint32_t))
    class HandleStorage final
    {
    private:
        using SlotType = uint32_t;
        using VersionType = uint32_t;

    public:
        HandleStorage() { GrowChunks(); }
        HandleStorage(const HandleStorage&) = delete;
        HandleStorage(HandleStorage&&) noexcept = delete;
        ~HandleStorage()
        {
            if (freeSlots.size() != slotCapacity)
            {
                for (const auto slot : views::iota(0Ui32, slotCapacity))
                {
                    if (!IsMarkedAsFreeSlot(slot))
                    {
                        Ty* slotElementPtr = CalcAddressOfSlot(slot);
                        slotElementPtr->~Ty();

#ifdef IG_TRACK_LEAKED_HANDLE
                        PrintToDebugger("*** Found Leaked Handle!!! ***\n");
                        PrintToDebugger(CallStack::Dump(lastCallStackTable[slot]));
                        PrintToDebugger("\n");
#endif
                        IG_CHECK_NO_ENTRY();
                    }
                }
            }

            for (uint8_t* chunk : chunks)
            {
                if (chunk != nullptr)
                {
                    _aligned_free(chunk);
                }
            }
        }

        HandleStorage& operator=(const HandleStorage&) = delete;
        HandleStorage& operator=(HandleStorage&&) noexcept = delete;

        [[nodiscard]] size_t GetCapacity() const { return slotCapacity; }
        [[nodiscard]] size_t GetNumAllocated() const { return slotCapacity - freeSlots.size(); }

        template <typename... Args>
        Handle<Ty, Dependency> Create(Args&&... args)
        {
            if (freeSlots.empty() && !GrowChunks())
            {
                return Handle<Ty, Dependency>{};
            }
            IG_CHECK(!freeSlots.empty());

            Handle<Ty, Dependency> newHandle{ 0 };
            const SlotType newSlot = freeSlots.back();
            freeSlots.pop_back();
            IG_CHECK(IsMarkedAsFreeSlot(newSlot));

            Ty* const slotElementPtr = CalcAddressOfSlot(newSlot);
            ::new (slotElementPtr) Ty(std::forward<Args>(args)...);

            newHandle.Value = SetBits<0, SlotSizeInBits>(newHandle.Value, newSlot);
            newHandle.Value = SetBits<VersionOffset, VersionSizeInBits>(newHandle.Value, slotVersions[newSlot]);
            newHandle.Value = SetBits<TypeHashBitsOffset, TypeHashSizeInBits>(newHandle.Value, ReduceHashTo16Bits(TypeHash64<Ty>));

#ifdef IG_TRACK_LEAKED_HANDLE
            lastCallStackTable[newSlot] = CallStack::Capture();
#endif

            return newHandle;
        }

        void Destroy(const Handle<Ty, Dependency> handle)
        {
            if (handle.IsNull())
            {
                return;
            }

            if (ReduceHashTo16Bits(TypeHash64<Ty>) != MaskBits<TypeHashBitsOffset, TypeHashSizeInBits>(handle.Value))
            {
                return;
            }

            const SlotType slot = MaskBits<0, SlotSizeInBits, SlotType>(handle.Value);
            if (!IsSlotInRange(slot))
            {
                return;
            }

            if (IsMarkedAsFreeSlot(slot))
            {
                IG_CHECK_NO_ENTRY();
                return;
            }

            const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
            if (version != slotVersions[slot])
            {
                return;
            }

            /*
             * 만약 2^{VersionBits} 만큼 할당-해제가 발생해 버전 값에 오버플로우가 일어나는 것은, 실제로 맨 처음 할당 되었던 핸들 객체가
             * 더 이상 존재하지 않아서 충돌이 일어나기 힘든 조건이라고 가정.
             */
            slotVersions[slot] = (slotVersions[slot] + 1) % MaxVersion;

            Ty* slotElementPtr = CalcAddressOfSlot(slot);
            slotElementPtr->~Ty();
            MarkAsFreeSlot(slot);

            freeSlots.push_back(slot);
        }

        Ty* Lookup(const Handle<Ty, Dependency> handle)
        {
            if (handle.IsNull())
            {
                return nullptr;
            }

            if (ReduceHashTo16Bits(TypeHash64<Ty>) != MaskBits<TypeHashBitsOffset, TypeHashSizeInBits>(handle.Value))
            {
                return nullptr;
            }

            const SlotType slot = MaskBits<0, SlotSizeInBits, SlotType>(handle.Value);
            if (!IsSlotInRange(slot))
            {
                return nullptr;
            }

            if (IsMarkedAsFreeSlot(slot))
            {
                return nullptr;
            }

            const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
            if (version != slotVersions[slot])
            {
                return nullptr;
            }

            return CalcAddressOfSlot(slot);
        }

        const Ty* Lookup(const Handle<Ty, Dependency> handle) const
        {
            if (handle.IsNull())
            {
                return nullptr;
            }

            if (ReduceHashTo16Bits(TypeHash64<Ty>) != MaskBits<TypeHashBitsOffset, TypeHashSizeInBits>(handle.Value))
            {
                return nullptr;
            }

            const SlotType slot = MaskBits<0, SlotSizeInBits, SlotType>(handle.Value);
            if (!IsSlotInRange(slot))
            {
                return nullptr;
            }

            if (IsMarkedAsFreeSlot(slot))
            {
                return nullptr;
            }

            const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
            if (version != slotVersions[slot])
            {
                return nullptr;
            }

            return CalcAddressOfSlot(slot);
        }

    private:
        bool GrowChunks()
        {
            const size_t newChunkCapacity = GetNewChunkCapacity();
            if (newChunkCapacity == chunks.size())
            {
                return false;
            }

            const size_t numNewChunks = newChunkCapacity - chunks.size();
            chunks.reserve(newChunkCapacity);
            for ([[maybe_unused]] const auto _ : views::iota(0Ui64, numNewChunks))
            {
                chunks.emplace_back(static_cast<uint8_t*>(_aligned_malloc(ChunkSizeInBytes, std::hardware_destructive_interference_size)));
            }

            const uint32_t oldSlotCapacity = slotCapacity;
            slotCapacity = static_cast<uint32_t>(NumSlotsPerChunk * newChunkCapacity);
            freeSlots.reserve(slotCapacity);
            slotVersions.resize(slotCapacity);

            for (const SlotType newSlot : views::iota(oldSlotCapacity, slotCapacity) | views::reverse)
            {
                freeSlots.emplace_back(newSlot);
                MarkAsFreeSlot(newSlot);
            }

#ifdef IG_TRACK_LEAKED_HANDLE
            lastCallStackTable.resize(slotCapacity);
#endif

            return true;
        }

        [[nodiscard]] bool IsSlotInRange(const uint32_t slot) const { return slot < slotCapacity; }

        [[nodiscard]] size_t GetNewChunkCapacity() const
        {
            if (chunks.empty())
            {
                return InitialNumChunks;
            }

            return std::min(chunks.size() + chunks.size() / 2, MaxNumChunks);
        }

        [[nodiscard]] Ty* CalcAddressOfSlot(const uint32_t slot)
        {
            IG_CHECK(IsSlotInRange(slot));
            const size_t chunkIdx = slot / NumSlotsPerChunk;
            IG_CHECK(chunkIdx < chunks.size());
            const size_t slotIdxInChunk = slot % NumSlotsPerChunk;
            IG_CHECK(slotIdxInChunk < NumSlotsPerChunk);
            return reinterpret_cast<Ty*>(chunks[chunkIdx] + (slotIdxInChunk * SizeOfElement));
        }

        [[nodiscard]] const Ty* CalcAddressOfSlot(const uint32_t slot) const
        {
            IG_CHECK(IsSlotInRange(slot));
            const size_t chunkIdx = slot / NumSlotsPerChunk;
            IG_CHECK(chunkIdx < chunks.size());
            const size_t slotIdxInChunk = slot % NumSlotsPerChunk;
            IG_CHECK(slotIdxInChunk < NumSlotsPerChunk);
            return reinterpret_cast<const Ty*>(chunks[chunkIdx] + (slotIdxInChunk * SizeOfElement));
        }

        void MarkAsFreeSlot(const uint32_t slot)
        {
            IG_CHECK(IsSlotInRange(slot));
            using MagicNumberType = std::decay_t<decltype(FreeSlotMagicNumber)>;
            auto* const freeSlotMagicNumberPtr = reinterpret_cast<MagicNumberType*>(CalcAddressOfSlot(slot));
            IG_CHECK(freeSlotMagicNumberPtr != nullptr);
            *freeSlotMagicNumberPtr = FreeSlotMagicNumber;
        }

        [[nodiscard]] bool IsMarkedAsFreeSlot(const uint32_t slot) const
        {
            IG_CHECK(IsSlotInRange(slot));
            using MagicNumberType = std::decay_t<decltype(FreeSlotMagicNumber)>;
            const auto* const freeSlotMagicNumberPtr = reinterpret_cast<const MagicNumberType*>(CalcAddressOfSlot(slot));
            IG_CHECK(freeSlotMagicNumberPtr != nullptr);
            return *freeSlotMagicNumberPtr == FreeSlotMagicNumber;
        }

    private:
        constexpr static size_t ChunkSizeInBytes = details::GetHeuristicOptimalChunkSize<Ty>();

        /*
         * LSB 0~21     <22 bits>  : Slot Bit
         * LSB 22~47    <26 bits>  : Version Bits
         * LSB 48~63    <16 bits>  : Type Validation Bits
         */
        constexpr static size_t SlotSizeInBits = 22;
        constexpr static size_t VersionOffset = SlotSizeInBits;
        constexpr static size_t VersionSizeInBits = 26;
        constexpr static size_t TypeHashBitsOffset = VersionOffset + VersionSizeInBits;
        constexpr static size_t TypeHashSizeInBits = 16;
        static_assert(TypeHashBitsOffset + TypeHashSizeInBits == 64);

        constexpr static size_t SizeOfElement = sizeof(Ty);
        constexpr static size_t MaxNumSlots = Pow(2, SlotSizeInBits);
        constexpr static VersionType MaxVersion = Pow(2, VersionSizeInBits);
        constexpr static size_t NumSlotsPerChunk = ChunkSizeInBytes / SizeOfElement;
        static_assert(NumSlotsPerChunk <= MaxNumSlots);
        constexpr static size_t MaxNumChunks = MaxNumSlots / NumSlotsPerChunk;

        constexpr static uint32_t FreeSlotMagicNumber = 0xF3EE6102;

        constexpr static size_t InitialNumChunks = 4;
        eastl::vector<uint8_t*> chunks{};

        uint32_t slotCapacity = 0;
        eastl::vector<SlotType> freeSlots{};
        eastl::vector<VersionType> slotVersions{};

#ifdef IG_TRACK_LEAKED_HANDLE
        eastl::vector<DWORD> lastCallStackTable{};
#endif
    };
}    // namespace ig
