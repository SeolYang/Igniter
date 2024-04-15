#include <Igniter.h>
#include <Core/MemoryPool.h>

namespace ig
{
    MemoryPool::MemoryPool()
        : sizeOfElement(0)
        , alignOfElement(0)
        , numInitialElementPerChunk(0)
        , magicNumber(InvalidMagicNumber)
    {
    }

    MemoryPool::MemoryPool(const size_t   newSizeOfElement, const size_t     newAlignOfElement,
                           const uint16_t numElementPerChunk, const uint32_t numInitialChunk,
                           const uint16_t magicNumber)
        : sizeOfElement(newSizeOfElement)
        , alignOfElement(newAlignOfElement)
        , numInitialElementPerChunk(numElementPerChunk)
        , magicNumber(magicNumber == 0 ? Random<uint16_t>(0, (0xffff - 1)) : magicNumber)
    {
        IG_CHECK(
            sizeOfElement >= sizeof(uint64_t) &&
            "The size of element must be at least sizeof(uint64_t) due to 'a element aliveness ensuring mechanism'.");
        IG_CHECK(numElementPerChunk > 0);
        IG_CHECK(numInitialChunk > 0);
        chunks.reserve(numInitialChunk);

        for (uint32_t chunkIdx = 0; chunkIdx < numInitialChunk; ++chunkIdx)
        {
            CreateNewChunk();
        }
    }

    MemoryPool::MemoryPool(MemoryPool&& other) noexcept
        : sizeOfElement(std::exchange(other.sizeOfElement, 0))
        , alignOfElement(std::exchange(other.alignOfElement, 0))
        , numInitialElementPerChunk(std::exchange(other.numInitialElementPerChunk, 0Ui16))
        , magicNumber(std::exchange(other.magicNumber, InvalidMagicNumber))
        , chunks(std::move(other.chunks))
        , pool(std::move(other.pool))
    {
    }

    MemoryPool::~MemoryPool()
    {
        IG_CHECK((magicNumber == InvalidMagicNumber) || IsFull());
        for (uint8_t* chunk : chunks)
        {
            _aligned_free(chunk);
        }
    }

    uint64_t MemoryPool::Allocate()
    {
        if (pool.empty() && !CreateNewChunk())
        {
            IG_CHECK_NO_ENTRY();
            return InvalidHandle;
        }

        const uint64_t newHandle = pool.front();
        pool.pop();

        uint64_t* handleInUseFlagPtr = GetHandleInUseFlagPtr(newHandle);
        if (handleInUseFlagPtr != nullptr && (*handleInUseFlagPtr == HandleCurrentlyNotInUseFlag))
        {
            *handleInUseFlagPtr = 0;
        }
        else
        {
            IG_CHECK_NO_ENTRY();
            return InvalidHandle;
        }

        return newHandle;
    }

    void MemoryPool::Deallocate(const uint64_t handle)
    {
        if (IsAlive(handle))
        {
            uint64_t* handleInUseFlagPtr = GetHandleInUseFlagPtr(handle);
            if (handleInUseFlagPtr != nullptr)
            {
                *handleInUseFlagPtr = HandleCurrentlyNotInUseFlag;
            }
            else
            {
                IG_CHECK_NO_ENTRY();
            }

            pool.push(handle);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }

    bool MemoryPool::IsFull() const
    {
        return pool.size() == (chunks.size() * numInitialElementPerChunk);
    }

    bool MemoryPool::CreateNewChunk()
    {
        if (chunks.size() + 1 != MaxNumChunk)
        {
            const auto newChunkIdx = static_cast<uint32_t>(chunks.size());
            chunks.emplace_back(reinterpret_cast<uint8_t*>(_aligned_malloc(
                sizeOfElement * numInitialElementPerChunk, std::hardware_destructive_interference_size)));
            for (uint16_t elementIdx = 0; elementIdx < numInitialElementPerChunk; ++elementIdx)
            {
                const uint64_t newHandle = MakeHandle(magicNumber, newChunkIdx, elementIdx);
                IG_CHECK(newHandle != InvalidHandle);
                pool.push(newHandle);

                uint64_t* handleInUseFlagPtr = GetHandleInUseFlagPtr(newHandle);
                *handleInUseFlagPtr          = HandleCurrentlyNotInUseFlag;
            }

            return true;
        }

        return false;
    }
} // namespace ig
