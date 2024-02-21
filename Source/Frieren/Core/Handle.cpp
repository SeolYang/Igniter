#include <Core/Handle.h>
#include <Core/HandleManager.h>

namespace fe
{
	Handle::Handle(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType)
		: owner(&handleManager), handle(handleManager.Allocate(typeHashVal, sizeOfType, alignOfType))
	{
		check(owner != nullptr);
		check(handle != InvalidHandle);
		check(sizeOfType > 0);
		check(alignOfType > 0);
	}

	Handle::Handle(Handle&& other) noexcept
		: owner(std::exchange(other.owner, nullptr)), handle(std::exchange(other.handle, InvalidHandle))
	{
	}

	uint8_t* Handle::GetAddressOf(const uint64_t typeHashValue)
	{
		return owner->GetAddressOf(typeHashValue, handle);
	}

	const uint8_t* Handle::GetAddressOf(const uint64_t typeHashValue) const
	{
		return owner->GetAddressOf(typeHashValue, handle);
	}

	uint8_t* Handle::GetValidatedAddressOf(const uint64_t typeHashValue)
	{
		return owner->GetValidatedAddressOf(typeHashValue, handle);
	}

	const uint8_t* Handle::GetValidatedAddressOf(const uint64_t typeHashValue) const
	{
		return owner->GetValidatedAddressOf(typeHashValue, handle);
	}

	bool Handle::IsAlive(const uint64_t typeHashValue) const
	{
		return IsValid() && owner->IsAlive(typeHashValue, handle);
	}

	bool Handle::IsPendingDeallocation() const
	{
		return owner != nullptr && owner->IsPendingDeallocation(handle);
	}

	void Handle::Deallocate(const uint64_t typeHashValue)
	{
		if (IsValid())
		{
			owner->Deallocate(typeHashValue, handle);
		}
		else
		{
			checkNoEntry();
		}
	}

	Handle& Handle::operator=(Handle&& other) noexcept
	{
		owner = std::exchange(other.owner, nullptr);
		handle = std::exchange(other.handle, InvalidHandle);
		return *this;
	}

	void Handle::MarkAsPendingDeallocation(const uint64_t typeHashVal)
	{
		check(typeHashVal != InvalidHandle);
		owner->MaskAsPendingDeallocation(typeHashVal, handle);
	}
} // namespace fe::experimental