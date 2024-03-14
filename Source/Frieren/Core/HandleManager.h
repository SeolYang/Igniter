#pragma once
#include <Core/MemoryPool.h>
#include <Core/Mutex.h>
#include <Core/Container.h>
#include <Core/HashUtils.h>

namespace fe
{
	namespace details
	{
		class HandleImpl;
	}

	class HandleManager
	{
		friend class details::HandleImpl;

	public:
		HandleManager() = default;
		HandleManager(const HandleManager&) = delete;
		HandleManager(HandleManager&&) noexcept = delete;
		~HandleManager();

		HandleManager& operator=(const HandleManager&) = delete;
		HandleManager& operator=(HandleManager&&) = delete;

	private:
		uint64_t Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement);
		void Deallocate(const uint64_t typeHashVal, const uint64_t handle);
		void MaskAsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle);

		uint8_t* GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle)
		{
			check(typeHashVal != InvalidHashVal);
			check(memPools.contains(typeHashVal));
			return memPools[typeHashVal].GetAddressOf(handle);
		}

		const uint8_t* GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
		{
			check(typeHashVal != InvalidHashVal);
			check(memPools.contains(typeHashVal));
			const auto& memPool = memPools.at(typeHashVal);
			return memPool.GetAddressOf(handle);
		}

		/* If handle.alived == address existed. */
		uint8_t* GetAddressOf(const uint64_t typeHashVal, const uint64_t handle)
		{
			ReadOnlyLock lock{ mutex };
			return GetAddressOfUnsafe(typeHashVal, handle);
		}

		const uint8_t* GetAddressOf(const uint64_t typeHashVal, const uint64_t handle) const
		{
			ReadOnlyLock lock{ mutex };
			return GetAddressOfUnsafe(typeHashVal, handle);
		}

		/* Validated Address = handle.alived && !pending-deallocation */
		uint8_t* GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle)
		{
			ReadOnlyLock lock{ mutex };
			check(memPools.contains(typeHashVal));
			return !IsPendingDeallocationUnsafe(typeHashVal, handle) ? GetAddressOfUnsafe(typeHashVal, handle) : nullptr;
		}

		const uint8_t* GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle) const
		{
			ReadOnlyLock lock{ mutex };
			return !IsPendingDeallocationUnsafe(typeHashVal, handle) ? GetAddressOfUnsafe(typeHashVal, handle) : nullptr;
		}

		bool IsAlive(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle) const;

		bool IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeallocationUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;

	private:
		mutable SharedMutex mutex;

		/* key: Hash value of unique type.  */
		robin_hood::unordered_map<uint64_t, MemoryPool> memPools{};
		robin_hood::unordered_map<uint64_t, robin_hood::unordered_set<uint64_t>> pendingDeallocationSets;

		constexpr static size_t SizeOfChunkBytes = 65536;
		constexpr static uint32_t NumInitialChunkPerPool = 2;
	};
} // namespace fe