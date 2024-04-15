#include <Igniter.h>
#include <D3D12/GpuSync.h>

namespace ig
{
    GpuSync::GpuSync(ID3D12Fence& fence, const size_t syncPoint) : fence(&fence)
                                                                 , syncPoint(syncPoint)
    {
        IG_CHECK(IsValid());
    }

    bool GpuSync::operator<(const GpuSync& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) &&
                syncPoint < rhs.syncPoint;
    }

    bool GpuSync::operator>(const GpuSync& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) &&
                syncPoint > rhs.syncPoint;
    }

    bool GpuSync::operator>=(const GpuSync& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) &&
                syncPoint >= rhs.syncPoint;
    }

    bool GpuSync::operator!=(const GpuSync& rhs) const noexcept
    {
        return fence != rhs.fence;
    }

    bool GpuSync::operator==(const GpuSync& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) &&
                syncPoint == rhs.syncPoint;
    }

    bool GpuSync::operator<=(const GpuSync& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) &&
                syncPoint <= rhs.syncPoint;
    }

    size_t GpuSync::GetCompletedSyncPoint() const
    {
        if (fence != nullptr)
        {
            return fence->GetCompletedValue();
        }

        IG_CHECK_NO_ENTRY();
        return 0;
    }

    void GpuSync::WaitOnCpu()
    {
        IG_CHECK(IsValid());
        if (fence != nullptr && syncPoint > GetCompletedSyncPoint())
        {
            fence->SetEventOnCompletion(syncPoint, nullptr);
        }
    }
} // namespace ig
