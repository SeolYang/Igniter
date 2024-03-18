#include <Core/HandleManager.h>
#include <Core/Assert.h>
#include <Core/HashUtils.h>
#include <Core/Handle.h>

namespace ig
{
    HandleManager::~HandleManager()
    {
    }

    HandleManager::Statistics HandleManager::GetStatistics() const
    {
        ReadOnlyLock lock{ mutex };
        Statistics newStatistics{};
        for (const auto& memPoolPair : memPools)
        {
            ++newStatistics.NumMemoryPools;
            newStatistics.AllocatedChunksSizeInBytes += memPoolPair.second.GetAllocatedChunksSize();
            newStatistics.UsedSizeInBytes += memPoolPair.second.GetUsedSizeInBytes();
            newStatistics.NumAllocatedChunks += memPoolPair.second.GetNumAllocatedChunks();
            newStatistics.NumAllocatedHandles += memPoolPair.second.GetNumAllocations();
        }
        
        return newStatistics;
    }

    uint64_t HandleManager::Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement)
    {
        IG_CHECK(typeHashVal != InvalidHashVal);

        ReadWriteLock lock{ mutex };
        if (!memPools.contains(typeHashVal))
        {
            memPools.insert({ typeHashVal,
                              MemoryPool{ sizeOfElement, alignOfElement, static_cast<uint16_t>(SizeOfChunkBytes / sizeOfElement), NumInitialChunkPerPool, static_cast<uint16_t>(typeHashVal) } });
        }
        IG_CHECK(memPools.contains(typeHashVal));

        const uint64_t allocatedHandle = memPools[typeHashVal].Allocate();
        IG_CHECK(allocatedHandle != details::HandleImpl::InvalidHandle);
        IG_CHECK(!IsPendingDeallocationUnsafe(typeHashVal, allocatedHandle));
        return allocatedHandle;
    }

    void HandleManager::Deallocate(const uint64_t typeHashVal, const uint64_t handle)
    {
        IG_CHECK(typeHashVal != InvalidHashVal);
        IG_CHECK(handle != details::HandleImpl::InvalidHandle);

        ReadWriteLock lock{ mutex };
        const bool bMemPoolExists = memPools.contains(typeHashVal);
        const bool bDeallocationSetExists = pendingDeallocationSets.contains(typeHashVal);
        if (bMemPoolExists && bDeallocationSetExists)
        {
            pendingDeallocationSets.at(typeHashVal).erase(handle);
            memPools[typeHashVal].Deallocate(handle);
        }
        else
        {
            IG_CHECK("Trying to deallocate invalid type of handle.");
        }
    }

    bool HandleManager::IsAlive(const uint64_t typeHashVal, const uint64_t handle) const
    {
        ReadOnlyLock lock{ mutex };
        return IsAliveUnsafe(typeHashVal, handle);
    }

    bool HandleManager::IsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle) const
    {
        ReadOnlyLock lock{ mutex };
        return IsPendingDeallocationUnsafe(typeHashVal, handle);
    }

    bool HandleManager::IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
    {
        IG_CHECK(typeHashVal != InvalidHashVal);
        IG_CHECK(handle != details::HandleImpl::InvalidHandle);
        const bool bPendingDeallocations = IsPendingDeallocationUnsafe(typeHashVal, handle);
        const bool bMemPoolExists = memPools.contains(typeHashVal);
        if (!bPendingDeallocations && bMemPoolExists)
        {
            const MemoryPool& pool = memPools.at(typeHashVal);
            return pool.IsAlive(handle);
        }

        return false;
    }

    bool HandleManager::IsPendingDeallocationUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
    {
        IG_CHECK(handle != details::HandleImpl::InvalidHandle);
        IG_CHECK(typeHashVal != InvalidHashVal);
        return pendingDeallocationSets.contains(typeHashVal) ? pendingDeallocationSets.at(typeHashVal).contains(handle) : false;
    }

    void HandleManager::MaskAsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle)
    {
        ReadWriteLock lock{ mutex };
        if (IsAliveUnsafe(typeHashVal, handle))
        {
            pendingDeallocationSets[typeHashVal].insert(handle);
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }
    }
} // namespace ig