#include <Core/HandleManager.h>
#include <Core/Assert.h>
#include <Core/HashUtils.h>
#include <Core/Handle.h>

namespace fe
{
	HandleManager::~HandleManager()
	{
		check(pendingDeallocations.empty());
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
		check(allocatedHandle != Handle::InvalidHandle);
		check(!pendingDeallocations.contains(allocatedHandle));
		return allocatedHandle;
	}

	void HandleManager::Deallocate(const uint64_t typeHashVal, const uint64_t handle)
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != Handle::InvalidHandle);

		ReadWriteLock lock{ mutex };
		if (memPools.contains(typeHashVal))
		{
			pendingDeallocations.erase(handle);
			memPools[typeHashVal].Deallocate(handle);
		}
		else
		{
			check("Trying to deallocate invalid type of handle.");
		}
	}

	uint8_t* HandleManager::GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle)
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != Handle::InvalidHandle);

		if (memPools.contains(typeHashVal))
		{
			return memPools[typeHashVal].GetAddressOf(handle);
		}

		return nullptr;
	}

	const uint8_t* HandleManager::GetAddressOfUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != Handle::InvalidHandle);

		if (memPools.contains(typeHashVal))
		{
			const auto& memPool = memPools.at(typeHashVal);
			return memPool.GetAddressOf(handle);
		}

		return nullptr;
	}

	uint8_t* HandleManager::GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle)
	{
		ReadOnlyLock lock{ mutex };
		return !pendingDeallocations.contains(handle) ? GetAddressOfUnsafe(typeHashVal, handle) : nullptr;
	}

	const uint8_t* HandleManager::GetValidatedAddressOf(const uint64_t typeHashVal, const uint64_t handle) const
	{
		ReadOnlyLock lock{ mutex };
		return !pendingDeallocations.contains(handle) ? GetAddressOfUnsafe(typeHashVal, handle) : nullptr;
	}

	bool HandleManager::IsAlive(const uint64_t typeHashVal, const uint64_t handle) const
	{
		ReadOnlyLock lock{ mutex };
		return IsAliveUnsafe(typeHashVal, handle);
	}

	bool HandleManager::IsPendingDeallocation(const uint64_t handle) const
	{
		ReadOnlyLock lock{ mutex };
		return IsPendingDeallocationUnsafe(handle);
	}

	bool HandleManager::IsAliveUnsafe(const uint64_t typeHashVal, const uint64_t handle) const
	{
		check(typeHashVal != InvalidHashVal);
		check(handle != Handle::InvalidHandle);
		if (!pendingDeallocations.contains(handle) && memPools.contains(typeHashVal))
		{
			const MemoryPool& pool = memPools.at(typeHashVal);
			return pool.IsAlive(handle);
		}

		return false;
	}

	bool HandleManager::IsPendingDeallocationUnsafe(const uint64_t handle) const
	{
		check(handle != Handle::InvalidHandle);
		return pendingDeallocations.contains(handle);
	}

	void HandleManager::MaskAsPendingDeallocation(const uint64_t typeHashVal, const uint64_t handle)
	{
		ReadWriteLock lock{ mutex };
		if (IsAliveUnsafe(typeHashVal, handle))
		{
			pendingDeallocations.insert(handle);
		}
		else
		{
			checkNoEntry();
		}
	}
} // namespace fe