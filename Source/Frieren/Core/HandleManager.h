#pragma once
#include <Core/MemoryPool.h>
#include <Core/Mutex.h>
#include <Core/Container.h>

namespace fe
{
	class HandleManager
	{
		friend class Handle;

	public:
		HandleManager() = default;
		~HandleManager();

	private:
		uint64_t Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement);
		void	 Deallocate(const uint64_t typeHashVal, const uint64_t handle);
		void	 MaskAsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle);

		uint8_t*	   GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle);
		const uint8_t* GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;

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
		uint8_t*	   GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle);
		const uint8_t* GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle) const;

		bool IsAlive(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeallocation(const uint64_t handle) const;

		bool IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeallocationUnsafe(const uint64_t handle) const;

	private:
		mutable SharedMutex mutex;

		/* key: Hash value of unique type.  */
		robin_hood::unordered_map<uint64_t, MemoryPool> memPools{};
		robin_hood::unordered_set<uint64_t>				pendingDeallocations{};

		constexpr static size_t	  SizeOfChunkBytes = 65536;
		constexpr static uint32_t NumInitialChunkPerPool = 2;
	};
} // namespace fe