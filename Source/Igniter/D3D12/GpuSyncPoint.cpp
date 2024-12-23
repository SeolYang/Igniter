#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    GpuSyncPoint::GpuSyncPoint(ID3D12Fence& fence, const size_t syncPoint) :
        fence(&fence),
        syncPoint(syncPoint)
    {
        IG_CHECK(IsValid());
    }

    bool GpuSyncPoint::operator<(const GpuSyncPoint& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) && syncPoint < rhs.syncPoint;
    }

    bool GpuSyncPoint::operator>(const GpuSyncPoint& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) && syncPoint > rhs.syncPoint;
    }

    bool GpuSyncPoint::operator>=(const GpuSyncPoint& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) && syncPoint >= rhs.syncPoint;
    }

    bool GpuSyncPoint::operator!=(const GpuSyncPoint& rhs) const noexcept
    {
        return fence != rhs.fence;
    }

    bool GpuSyncPoint::operator==(const GpuSyncPoint& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) && syncPoint == rhs.syncPoint;
    }

    bool GpuSyncPoint::operator<=(const GpuSyncPoint& rhs) const noexcept
    {
        return (fence != nullptr && (fence == rhs.fence)) && syncPoint <= rhs.syncPoint;
    }

    size_t GpuSyncPoint::GetCompletedSyncPoint() const
    {
        IG_CHECK(fence != nullptr);
        if (!IsValid())
        {
            return std::numeric_limits<size_t>::max();
        }

        return fence->GetCompletedValue();
    }

    GpuSyncPoint GpuSyncPoint::Prev() const
    {
        if (Invalid() || syncPoint == 1)
        {
            return Invalid();
        }

        return GpuSyncPoint{ *fence, syncPoint - 1 };
    }

    void GpuSyncPoint::WaitOnCpu()
    {
        if (fence != nullptr && syncPoint > GetCompletedSyncPoint())
        {
            fence->SetEventOnCompletion(syncPoint, nullptr);
        }
    }
}    // namespace ig
