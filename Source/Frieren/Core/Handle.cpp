#include <Core/Handle.h>
#include <Core/HandleManager.h>

namespace fe
{
	HandleImpl::HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType)
		: owner(&handleManager), handle(handleManager.Allocate(typeHashVal, sizeOfType, alignOfType))
	{
		check(owner != nullptr);
		check(handle != InvalidHandle);
		check(sizeOfType > 0);
		check(alignOfType > 0);
	}

	HandleImpl::HandleImpl(HandleImpl&& other) noexcept
		: owner(std::exchange(other.owner, nullptr)), handle(std::exchange(other.handle, InvalidHandle))
	{
	}

	uint8_t* HandleImpl::GetAddressOf(const uint64_t typeHashValue)
	{
		check(IsValid());
		check(typeHashValue != InvalidHashVal);
		return owner->GetAddressOf(typeHashValue, handle);
	}

	const uint8_t* HandleImpl::GetAddressOf(const uint64_t typeHashValue) const
	{
		check(IsValid());
		check(typeHashValue != InvalidHashVal);
		return owner->GetAddressOf(typeHashValue, handle);
	}

	bool HandleImpl::IsAlive(const uint64_t typeHashValue) const
	{
		return IsValid() && owner->IsAlive(typeHashValue, handle);
	}

	bool HandleImpl::IsPendingDeferredDeallocation() const
	{
		return owner != nullptr && owner->IsPendingDeferredDeallocation(handle);
	}

	void HandleImpl::Deallocate(const uint64_t typeHashValue)
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

	HandleImpl& HandleImpl::operator=(HandleImpl&& other) noexcept
	{
		owner = std::exchange(other.owner, nullptr);
		handle = std::exchange(other.handle, InvalidHandle);
		return *this;
	}

	void HandleImpl::RequestDeferredDeallocation(const uint64_t typeHashVal)
	{
		check(typeHashVal != InvalidHandle);
		owner->RequestDeferredDeallocation(typeHashVal, handle);
	}
} // namespace fe::experimental