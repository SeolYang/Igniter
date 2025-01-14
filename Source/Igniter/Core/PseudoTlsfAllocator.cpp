#include "Igniter/Igniter.h"
#include "Igniter/Core/PseudoTlsfAllocator.h"

namespace ig
{
    constexpr inline size_t InvalidTLSFLevelIdx = (size_t)-1;

    static size_t MapFirstLevelIndex(const size_t allocSize)
    {
        return std::min<uint8_t>(64Ui8 - (uint8_t)std::countl_zero(allocSize) - 1Ui8, 63Ui8);
    }

    static size_t MapSecondLevelIndex(const size_t allocSize, const size_t firstLevelIdx, const size_t secondLevelParam)
    {
        return ((allocSize ^ (1Ui64 << firstLevelIdx))) >> (firstLevelIdx - secondLevelParam);
    }

    PseudoTlsfAllocator::PseudoTlsfAllocator(const size_t memoryPoolSize, const size_t secondLevelParam)
        : memoryPoolSize(memoryPoolSize)
        , firstLevelParam(MapFirstLevelIndex(memoryPoolSize))
        , secondLevelParam(secondLevelParam)
        , numSubdivisions(1Ui64 << secondLevelParam)
    {
        secondLevelBitmaps.resize(firstLevelParam + 1Ui64);
        freeLists.resize(firstLevelParam * numSubdivisions + 1Ui64);

        details::PseudoTlsfBlockHandle newBlockHandle = blockRegistry.Create();
        details::PseudoTlsfBlock* newBlock = blockRegistry.Lookup(newBlockHandle);
        newBlock->Offset = 0Ui64;
        newBlock->Size = memoryPoolSize;
        Insert(newBlockHandle);
    }

    PseudoTlsfAllocator::~PseudoTlsfAllocator()
    {
        for (details::PseudoTlsfBlockHandle headBlockHandle : freeLists)
        {
            if (headBlockHandle.IsNull())
            {
                continue;
            }

            details::PseudoTlsfBlock* headBlock = blockRegistry.Lookup(headBlockHandle);
            IG_CHECK(headBlock != nullptr);

            details::PseudoTlsfBlockHandle nextBlockHandle = headBlock->NextFreeBlock;
            while (!nextBlockHandle.IsNull())
            {
                details::PseudoTlsfBlock* nextBlock = blockRegistry.Lookup(nextBlockHandle);
                IG_CHECK(nextBlock != nullptr);
                nextBlockHandle = nextBlock->NextFreeBlock;
                blockRegistry.Destroy(nextBlockHandle);
            }

            blockRegistry.Destroy(headBlockHandle);
        }
    }
} // namespace ig
