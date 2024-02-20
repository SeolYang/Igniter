#include <Core/HandleManager.h>
#include <Core/Assert.h>
#include <Core/HashUtils.h>
#include <Core/Handle.h>

namespace fe
{
	HandleManager::~HandleManager()
	{
		check(deferredDeallocationHandles.empty());
	}

	uint64_t HandleManager::Allocate(const uint64_t typeHashVal, const size_t sizeOfElement, const size_t alignOfElement)
	{
		check(typeHashVal != InvalidHashVal);

		ReadWriteLock lock{ mutex };
		if (!memPools.contains(typeHashVal))
		{
			memPools.insert({ typeHashVal,
							  MemoryPool{ sizeOfElement, alignOfElement, static_cast<uint16_t>(SizeOfChunkBytes / sizeOfElement), NumInitialChunkPerPool } });
		}
		check(memPools.contains(typeHashVal));

		const uint64_t allocatedHandle = memPools[typeHashVal].Allocate();
		check(allocatedHandle != HandleImpl::InvalidHandle);
		check(!deferredDeallocationHandles.contains(allocatedHandle));
		return allocatedHandle;
	}

	void HandleManager::Deallocate(const uint64_t typeHashVal, const uint64_t handle)
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != HandleImpl::InvalidHandle);

		ReadWriteLock lock{ mutex };
		if (memPools.contains(typeHashVal))
		{
			deferredDeallocationHandles.erase(handle);
			memPools[typeHashVal].Deallocate(handle);
		}
		else
		{
			check("Trying to deallocate invalid type of handle.");
		}
	}

	uint8_t* HandleManager::GetAddressOf(const uint64_t typeHashVal, const uint64_t handle)
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != HandleImpl::InvalidHandle);

		ReadOnlyLock lock{ mutex };
		if (!deferredDeallocationHandles.contains(handle) && memPools.contains(typeHashVal))
		{
			return memPools[typeHashVal].GetAddressOf(handle);
		}

		return nullptr;
	}

	const uint8_t* HandleManager::GetAddressOf(const uint64_t typeHashVal, const uint64_t handle) const
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != HandleImpl::InvalidHandle);

		ReadOnlyLock lock{ mutex };
		if (!deferredDeallocationHandles.contains(handle) && memPools.contains(typeHashVal))
		{
			const MemoryPool& pool = memPools.at(typeHashVal);
			return pool.GetAddressOf(handle);
		}

		return nullptr;
	}

	bool HandleManager::IsAlive(const uint64_t typeHashVal, const uint64_t handle) const
	{
		ReadOnlyLock lock{ mutex };
		return IsAliveUnsafe(typeHashVal, handle);
	}

	bool HandleManager::IsPendingDeferredDeallocation(const uint64_t handle) const
	{
		ReadOnlyLock lock{ mutex };
		return IsPendingDeferredDeallocationUnsafe(handle);
	}

	bool HandleManager::IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != HandleImpl::InvalidHandle);
		if (!deferredDeallocationHandles.contains(handle) && memPools.contains(typeHashVal))
		{
			const MemoryPool& pool = memPools.at(typeHashVal);
			return pool.IsAlive(handle);
		}

		return false;
	}

	bool HandleManager::IsPendingDeferredDeallocationUnsafe(const uint64_t handle) const
	{
		check(handle != HandleImpl::InvalidHandle);
		return deferredDeallocationHandles.contains(handle);
	}

	void HandleManager::RequestDeferredDeallocation(const uint64_t typeHashVal, const uint64_t handle)
	{
		ReadWriteLock lock{ mutex };
		if (IsAliveUnsafe(typeHashVal, handle))
		{
			deferredDeallocationHandles.insert(handle);
		}
		else
		{
			checkNoEntry();
		}
	}
} // namespace fe