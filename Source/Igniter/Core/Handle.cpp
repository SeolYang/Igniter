#include <Igniter.h>
#include <Core/Handle.h>
#include <Core/HandleManager.h>

namespace ig
{
    namespace details
    {
        HandleImpl::HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType,
                               const size_t   alignOfType)
            : owner(&handleManager)
            , handle(handleManager.Allocate(typeHashVal, sizeOfType, alignOfType))
        {
            IG_CHECK(owner != nullptr);
            IG_CHECK(handle != InvalidHandle);
            IG_CHECK(sizeOfType > 0);
            IG_CHECK(alignOfType > 0);
        }

        HandleImpl::HandleImpl(HandleImpl&& other) noexcept
            : owner(std::exchange(other.owner, nullptr))
            , handle(std::exchange(other.handle, InvalidHandle))
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

        uint8_t* HandleImpl::GetAliveAddressOf(const uint64_t typeHashValue)
        {
            return owner->GetAliveAddressOf(typeHashValue, handle);
        }

        const uint8_t* HandleImpl::GetAliveAddressOf(const uint64_t typeHashValue) const
        {
            return owner->GetAliveAddressOf(typeHashValue, handle);
        }

        bool HandleImpl::IsAlive(const uint64_t typeHashValue) const
        {
            return IsValid() && owner->IsAlive(typeHashValue, handle);
        }

        void HandleImpl::Deallocate(const uint64_t typeHashValue)
        {
            if (IsValid())
            {
                owner->Deallocate(typeHashValue, handle);
            }
            else
            {
                IG_CHECK_NO_ENTRY();
            }
        }

        HandleImpl& HandleImpl::operator=(HandleImpl&& other) noexcept
        {
            owner  = std::exchange(other.owner, nullptr);
            handle = std::exchange(other.handle, InvalidHandle);
            return *this;
        }

        void HandleImpl::MarkAsPendingDeallocation(const uint64_t typeHashVal)
        {
            IG_CHECK(typeHashVal != InvalidHandle);
            owner->MaskAsPendingDeallocation(typeHashVal, handle);
        }
    } // namespace details
}     // namespace ig
