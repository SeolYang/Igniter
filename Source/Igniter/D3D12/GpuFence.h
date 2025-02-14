#pragma once
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    class GpuFence final
    {
        friend class GpuDevice;
        friend class CommandQueue;

    public:
        GpuFence(const GpuFence& other) = delete;

        GpuFence(GpuFence&& other) noexcept
            : counter(other.counter.exchange(1))
            , fence(std::move(other.fence))
        {}

        ~GpuFence() = default;

        GpuFence& operator=(const GpuFence&) = delete;

        GpuFence& operator=(GpuFence&& other) noexcept
        {
            this->counter = other.counter.exchange(1);
            this->fence = std::move(other.fence);
            return *this;
        }

        [[nodiscard]] ID3D12Fence& GetNative() { return *fence.Get(); }
        [[nodiscard]] const ID3D12Fence& GetNative() const { return *fence.Get(); }
        [[nodiscard]] bool IsValid() const noexcept { return fence; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        GpuSyncPoint MakeSyncPoint()
        {
            const U64 syncPointCounter{counter.fetch_add(1)};
            IG_CHECK(syncPointCounter >= 1);
            return GpuSyncPoint{*fence.Get(), syncPointCounter};
        }

    private:
        GpuFence(ComPtr<ID3D12Fence> fence)
            : fence(std::move(fence))
        {}

    private:
        std::atomic_uint64_t counter{1};
        ComPtr<ID3D12Fence> fence;
    };
} // namespace ig
