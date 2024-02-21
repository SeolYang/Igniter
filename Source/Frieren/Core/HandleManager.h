#pragma once
#include <Core/MemoryPool.h>
#include <Core/Mutex.h>
#include <Core/Container.h>

namespace fe
{
	class HandleManager
	{
		friend class HandleImpl;

	public:
		HandleManager() = default;
		~HandleManager();

	private:
		uint64_t Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement);
		void	 Deallocate(const uint64_t typeHashVal, const uint64_t handle);
		void	 RequestDeferredDeallocation(const uint64_t typeHashVal, const uint64_t handle);

		uint8_t*	   GetAddressOf(const uint64_t typeHashVal, const uint64_t handle);
		const uint8_t* GetAddressOf(const uint64_t typeHashVal, const uint64_t handle) const;

		bool IsAlive(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeferredDeallocation(const uint64_t handle) const;

		bool IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const;
		bool IsPendingDeferredDeallocationUnsafe(const uint64_t handle) const;

	private:
		mutable SharedMutex mutex;

		/* key: Hash value of unique type.  */
		robin_hood::unordered_map<uint64_t, MemoryPool> memPools{};
		robin_hood::unordered_set<uint64_t>				deferredDeallocationHandles{};

		constexpr static size_t	  SizeOfChunkBytes = 65536;
		constexpr static uint32_t NumInitialChunkPerPool = 2;
	};
} // namespace fe