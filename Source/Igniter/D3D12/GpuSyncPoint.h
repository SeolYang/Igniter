#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class CommandQueue;

    class GpuSyncPoint final
    {
        friend class GpuFence;

    public:
        GpuSyncPoint()                        = default;
        GpuSyncPoint(const GpuSyncPoint&)     = default;
        GpuSyncPoint(GpuSyncPoint&&) noexcept = default;
        ~GpuSyncPoint()                       = default;

        GpuSyncPoint& operator=(const GpuSyncPoint& rhs) = default;
        GpuSyncPoint& operator=(GpuSyncPoint&&) noexcept = default;

        bool operator<(const GpuSyncPoint& rhs) const noexcept;
        bool operator>(const GpuSyncPoint& rhs) const noexcept;
        bool operator<=(const GpuSyncPoint& rhs) const noexcept;
        bool operator>=(const GpuSyncPoint& rhs) const noexcept;
        bool operator==(const GpuSyncPoint& rhs) const noexcept;
        bool operator!=(const GpuSyncPoint& rhs) const noexcept;

        [[nodiscard]] bool IsValid() const noexcept { return fence != nullptr && syncPoint > 0; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        [[nodiscard]] size_t GetSyncPoint() const noexcept { return syncPoint; }
        [[nodiscard]] size_t GetCompletedSyncPoint() const;
        [[nodiscard]] bool   IsExpired() const { return GetSyncPoint() <= GetCompletedSyncPoint(); }

        [[nodiscard]] ID3D12Fence& GetFence()
        {
            IG_CHECK(fence != nullptr);
            return *fence;
        }

        [[nodiscard]] GpuSyncPoint Prev() const;

        void WaitOnCpu();

        static GpuSyncPoint Invalid() { return { }; }

    private:
        GpuSyncPoint(ID3D12Fence& fence, const size_t syncPoint);

    private:
        ID3D12Fence* fence     = nullptr;
        size_t       syncPoint = 0;
    };
} // namespace ig
