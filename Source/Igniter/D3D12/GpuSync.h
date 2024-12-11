#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class CommandQueue;
    class GpuSync final
    {
        friend class CommandQueue;

    public:
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
        [[nodiscard]] operator bool() const { return IsValid(); }

        [[nodiscard]] size_t GetSyncPoint() const noexcept { return syncPoint; }
        [[nodiscard]] size_t GetCompletedSyncPoint() const;
        [[nodiscard]] bool IsExpired() const { return GetSyncPoint() <= GetCompletedSyncPoint(); }

        void WaitOnCpu();

        static GpuSync Invalid() { return {}; }

    private:
        GpuSync(ID3D12Fence& fence, const size_t syncPoint);

        [[nodiscard]] ID3D12Fence& GetFence()
        {
            IG_CHECK(fence != nullptr);
            return *fence;
        }

    private:
        ID3D12Fence* fence = nullptr;
        size_t syncPoint = 0;
    };
}    // namespace ig
