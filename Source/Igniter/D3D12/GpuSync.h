#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuSync final
    {
    public:
        GpuSync(ID3D12Fence& fence, const size_t syncPoint);
        GpuSync() = default;
        GpuSync(const GpuSync&) = default;
        GpuSync(GpuSync&&) noexcept = default;
        ~GpuSync() = default;

        GpuSync& operator=(const GpuSync& rhs) = default;
        GpuSync& operator=(GpuSync&&) noexcept = default;

        bool operator<(const GpuSync& rhs) const noexcept;
        bool operator>(const GpuSync& rhs) const noexcept;
        bool operator<=(const GpuSync& rhs) const noexcept;
        bool operator>=(const GpuSync& rhs) const noexcept;
        bool operator==(const GpuSync& rhs) const noexcept;
        bool operator!=(const GpuSync& rhs) const noexcept;

        [[nodiscard]] bool IsValid() const noexcept { return fence != nullptr && syncPoint > 0; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }
        [[nodiscard]] ID3D12Fence& GetFence()
        {
            IG_CHECK(fence != nullptr);
            return *fence;
        }

        [[nodiscard]] size_t GetSyncPoint() const noexcept { return syncPoint; }
        [[nodiscard]] size_t GetCompletedSyncPoint() const;
        [[nodiscard]] bool IsExpired() const { return GetSyncPoint() <= GetCompletedSyncPoint(); }

        void WaitOnCpu();

        static GpuSync Invalid() { return {}; }

    private:
        ID3D12Fence* fence = nullptr;
        size_t syncPoint = 0;
    };
}    // namespace ig
