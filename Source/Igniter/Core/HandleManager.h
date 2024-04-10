#pragma once
#include <Igniter.h>
#include <Core/MemoryPool.h>

namespace ig
{
    namespace details
    {
        class HandleImpl;
    }

    class HandleManager final
    {
        friend class details::HandleImpl;

    public:
        struct Statistics
        {
            size_t NumMemoryPools = 0;
            size_t AllocatedChunksSizeInBytes = 0;
            size_t UsedSizeInBytes = 0;
            size_t NumAllocatedChunks = 0;
            size_t NumAllocatedHandles = 0;
        };

    public:
        HandleManager() = default;
        HandleManager(const HandleManager&) = delete;
        HandleManager(HandleManager&&) noexcept = delete;
        ~HandleManager() = default;

        HandleManager& operator=(const HandleManager&) = delete;
        HandleManager& operator=(HandleManager&&) = delete;

        Statistics GetStatistics() const;

    private:
        uint64_t Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement);
        void Deallocate(const uint64_t typeHashVal, const uint64_t handle);
        void MaskAsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle);

        uint8_t* GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle);
        const uint8_t* GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;

        /* If handle.alived == address existed. */
        uint8_t* GetAddressOf(const uint64_t typeHashVal, const uint64_t handle);
        const uint8_t* GetAddressOf(const uint64_t typeHashVal, const uint64_t handle) const;

        /* Validated Address = handle.alived && !pending-deallocation */
        uint8_t* GetAliveAddressOf(const uint64_t typeHashVal, const uint64_t handle);
        const uint8_t* GetAliveAddressOf(const uint64_t typeHashVal, const uint64_t handle) const;

        bool IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;
        bool IsAlive(const uint64_t typeHashVal, const uint64_t handle) const;

        bool IsPendingDeallocationUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;
        bool IsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle) const;

    private:
        mutable SharedMutex mutex;

        /* key: Hash value of unique type.  */
        UnorderedMap<uint64_t, MemoryPool> memPools{};
        UnorderedMap<uint64_t, UnorderedSet<uint64_t>> pendingDeallocationSets;

        constexpr static size_t SizeOfChunkBytes = 65536;
        constexpr static uint32_t NumInitialChunkPerPool = 2;
    };
} // namespace ig