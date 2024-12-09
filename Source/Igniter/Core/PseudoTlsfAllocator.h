#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/HandleRegistry.h"

namespace ig
{
    class PseudoTlsfAllocator;
    namespace details
    {
        struct PseudoTlsfBlock;
        using PseudoTlsfBlockHandle = Handle<details::PseudoTlsfBlock, PseudoTlsfAllocator>;

        struct PseudoTlsfBlock
        {
            size_t Offset = 0;
            size_t Size : 63 = 0;
            size_t bIsUsed : 1 = false;

            PseudoTlsfBlockHandle PrevPhysicalBlock{};
            PseudoTlsfBlockHandle NextPhysicalBlock{};
            PseudoTlsfBlockHandle PrevFreeBlock{};
            PseudoTlsfBlockHandle NextFreeBlock{};
        };
    }    // namespace details

    class PseudoTlsfAllocator;
    struct PseudoTlsfAllocation
    {
        details::PseudoTlsfBlockHandle Block{};
        size_t Offset = 0;
        size_t Size = 0;
    };

    class PseudoTlsfAllocator
    {
    public:
        PseudoTlsfAllocator(const size_t memoryPoolSize, const size_t secondLevelParam);
        PseudoTlsfAllocator(const PseudoTlsfAllocator&) = delete;
        PseudoTlsfAllocator(PseudoTlsfAllocator&&) noexcept = delete;
        ~PseudoTlsfAllocator();

        PseudoTlsfAllocator& operator=(const PseudoTlsfAllocator&) = delete;
        PseudoTlsfAllocator& operator=(PseudoTlsfAllocator&&) noexcept = delete;

        PseudoTlsfAllocation Allocate(const size_t allocSize);
        void Deallocate(PseudoTlsfAllocation allocation);

    private:
        void Insert(Handle<details::PseudoTlsfBlock, PseudoTlsfAllocator> block);
        details::PseudoTlsfBlockHandle ExtractHead(const size_t firstLevelIdx, const size_t secondLevelIdx);
        void Extract(Handle<details::PseudoTlsfBlock, PseudoTlsfAllocator> block);

    private:
        HandleRegistry<details::PseudoTlsfBlock, PseudoTlsfAllocator> blockRegistry;

        size_t memoryPoolSize = 0;

        size_t firstLevelParam = 0;
        uint64_t firstLevelBitmap = 0;

        size_t secondLevelParam = 0;
        size_t numSubdivisions = 0;

        eastl::vector<uint64_t> secondLevelBitmaps;
        eastl::vector<details::PseudoTlsfBlockHandle> freeLists;
    };
}    // namespace ig