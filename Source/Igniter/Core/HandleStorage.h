#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Memory.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Handle.h"
#ifdef IG_TRACK_LEAKED_HANDLE
#include "Igniter/Core/DebugTools.h"
#endif

IG_DECLARE_LOG_CATEGORY(HandleStorageLog);

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
} // namespace ig::details

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
                IG_LOG(HandleStorageLog, Fatal, "{} handles are leaked!!!", (slotCapacity - freeSlots.size()));
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

            Handle<Ty, Dependency> newHandle{0};
            const SlotType newSlot = freeSlots.back();
            freeSlots.pop_back();
            IG_CHECK(IsMarkedAsFreeSlot(newSlot));

            Ty* const slotElementPtr = CalcAddressOfSlot(newSlot);
            ::new (slotElementPtr) Ty(std::forward<Args>(args)...);

            newHandle.Value = SetBits<0, SlotSizeInBits>(newHandle.Value, newSlot);
            newHandle.Value = SetBits<VersionOffset, VersionSizeInBits>(newHandle.Value, slotVersions[newSlot]);
            IG_CHECK(!reservedToDestroyFlags[newSlot]);

#ifdef IG_TRACK_LEAKED_HANDLE
            lastCallStackTable[newSlot] = CallStack::Capture();
#endif

            return newHandle;
        }

        /*
         * 실제로 핸들을 해제하지 않고, 해제될 예정이라고 마킹만 해둔다.
         * 해제(Destroy) 전 까지 해당 핸들의 슬롯의 버전은 고정되고, 새롭게 핸들을 할당 하더라도
         * 마킹된 슬롯으로 새로운 핸들이 생성 되지 않는다.
         * 또한 Lookup 함수를 통한 데이터에 대한 접근이 허용되지 않는다.
         * LookupUnsafe 함수를 통해 명시적으로 해제 예정 마킹이된 핸들에 대한 데이터 접근이 가능하다.
         * 하지만 여전히 실제로 해제된 핸들에 대한 데이터 접근을 불가능 하다.
         * 이러한 매커니즘을 통해 지연된 리소스 해제와 같은 추가적인 기능을 구현 가능하다.
         */
        void MarkAsDestroy(const Handle<Ty, Dependency> handle)
        {
            if (handle.IsNull())
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

            if (const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
                version != slotVersions[slot])
            {
                return;
            }

            reservedToDestroyFlags[slot] = true;
        }

        void Destroy(const Handle<Ty, Dependency> handle)
        {
            if (handle.IsNull())
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

            if (const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
                version != slotVersions[slot])
            {
                return;
            }

            /*
             * 만약 2^{VersionBits} 만큼 할당-해제가 발생해 버전 값에 오버플로우가 일어나는 것은, 실제로 맨 처음 할당 되었던 핸들 객체가
             * 더 이상 존재하지 않아서 충돌이 일어나기 힘든 조건이라고 가정.
             */
            slotVersions[slot] = (slotVersions[slot] + 1) % MaxVersion;
            reservedToDestroyFlags[slot] = false;

            Ty* slotElementPtr = CalcAddressOfSlot(slot);
            slotElementPtr->~Ty();
            MarkAsFreeSlot(slot);

            freeSlots.push_back(slot);
        }

        // Destroy 예약 마킹이 되어있어도 데이터를 가져옴
        Ty* LookupUnsafe(const Handle<Ty, Dependency> handle)
        {
            if (handle.IsNull())
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

            if (const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
                version != slotVersions[slot])
            {
                return nullptr;
            }

            return CalcAddressOfSlot(slot);
        }

        const Ty* LookupUnsafe(const Handle<Ty, Dependency> handle) const
        {
            if (handle.IsNull())
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

            if (const VersionType version = MaskBits<VersionOffset, VersionSizeInBits, VersionType>(handle.Value);
                version != slotVersions[slot])
            {
                return nullptr;
            }

            return CalcAddressOfSlot(slot);
        }

        Ty* Lookup(const Handle<Ty, Dependency> handle)
        {
            Ty* const ptr = LookupUnsafe(handle);
            if (const SlotType slot = MaskBits<0, SlotSizeInBits, SlotType>(handle.Value);
                ptr != nullptr && reservedToDestroyFlags[slot])
            {
                return nullptr;
            }

            return ptr;
        }

        const Ty* Lookup(const Handle<Ty, Dependency> handle) const
        {
            const Ty* const ptr = LookupUnsafe(handle);
            if (const SlotType slot = MaskBits<0, SlotSizeInBits, SlotType>(handle.Value);
                ptr != nullptr && reservedToDestroyFlags[slot])
            {
                return nullptr;
            }

            return ptr;
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
            reservedToDestroyFlags.resize(slotCapacity);

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
         * LSB 0~29     <30 bits>  : Slot Bit
         * LSB 29~61    <32 bits>  : Version Bits
         * LSB 62~63    <2  bits>  : Reserved for future
         */
        constexpr static size_t SlotSizeInBits = 30;
        constexpr static size_t VersionOffset = SlotSizeInBits;
        constexpr static size_t VersionSizeInBits = 32;
        constexpr static size_t ReservedBits = 2;
        static_assert(VersionOffset + VersionSizeInBits + ReservedBits == 64);
        constexpr static size_t SizeOfElement = sizeof(Ty);
        constexpr static size_t MaxNumSlots = Pow<size_t>(2, SlotSizeInBits);
        constexpr static VersionType MaxVersion = Pow<VersionType>(2, VersionSizeInBits) - 1;
        constexpr static size_t NumSlotsPerChunk = ChunkSizeInBytes / SizeOfElement;
        static_assert(NumSlotsPerChunk <= MaxNumSlots);
        constexpr static size_t MaxNumChunks = MaxNumSlots / NumSlotsPerChunk;

        constexpr static uint32_t FreeSlotMagicNumber = 0xF3EE6102u;

        constexpr static size_t InitialNumChunks = 4;
        eastl::vector<uint8_t*> chunks{};

        uint32_t slotCapacity = 0;
        eastl::vector<SlotType> freeSlots{};
        eastl::vector<VersionType> slotVersions{};
        eastl::bitvector<> reservedToDestroyFlags{};

#ifdef IG_TRACK_LEAKED_HANDLE
        eastl::vector<DWORD> lastCallStackTable{};
#endif
    };
} // namespace ig
