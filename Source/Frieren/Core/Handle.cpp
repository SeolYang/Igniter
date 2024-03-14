#include <Core/Handle.h>
#include <Core/HandleManager.h>

namespace fe
{
	namespace details
	{
		HandleImpl::HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType)
			: owner(&handleManager),
			  handle(handleManager.Allocate(typeHashVal, sizeOfType, alignOfType))
		{
			check(owner != nullptr);
			check(handle != InvalidHandle);
			check(sizeOfType > 0);
			check(alignOfType > 0);
		}

		HandleImpl::HandleImpl(HandleImpl&& other) noexcept
			: owner(std::exchange(other.owner, nullptr)),
			  handle(std::exchange(other.handle, InvalidHandle))
		{
		}

		uint8_t* HandleImpl::GetAddressOf(const uint64_t typeHashValue)
		{
			return owner->GetAddressOf(typeHashValue, handle);
		}

		const uint8_t* HandleImpl::GetAddressOf(const uint64_t typeHashValue) const
		{
			return owner->GetAddressOf(typeHashValue, handle);
		}

		uint8_t* HandleImpl::GetValidatedAddressOf(const uint64_t typeHashValue)
		{
			return owner->GetValidatedAddressOf(typeHashValue, handle);
		}

		const uint8_t* HandleImpl::GetValidatedAddressOf(const uint64_t typeHashValue) const
		{
			return owner->GetValidatedAddressOf(typeHashValue, handle);
		}

		bool HandleImpl::IsAlive(const uint64_t typeHashValue) const
		{
			return IsValid() && owner->IsAlive(typeHashValue, handle);
		}

		bool HandleImpl::IsPendingDeallocation(const uint64_t typeHashValue) const
		{
			return owner != nullptr && owner->IsPendingDeallocation(typeHashValue, handle);
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

		void HandleImpl::MarkAsPendingDeallocation(const uint64_t typeHashVal)
		{
			check(typeHashVal != InvalidHandle);
			owner->MaskAsPendingDeallocation(typeHashVal, handle);
		}
	} // namespace details
} // namespace fe