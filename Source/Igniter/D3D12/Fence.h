#pragma once
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    class Fence final
    {
        friend class GpuDevice;
        friend class CommandQueue;

    public:
        Fence(const Fence& other) = delete;
        Fence(Fence&& other) noexcept :
            counter(other.counter.exchange(1)),
            fence(std::move(other.fence))
        {
        }

        ~Fence() = default;

        Fence& operator=(const Fence&) = delete;
        Fence& operator=(Fence&& other) noexcept
        {
            this->counter = other.counter.exchange(1);
            this->fence = std::move(other.fence);
            return *this;
        }

        ID3D12Fence& GetNative() { return *fence.Get(); }
        [[nodiscard]] bool IsValid() const noexcept { return fence; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        U64 IncreaseCounter() { return counter.fetch_add(1); }

    private:
        Fence(ComPtr<ID3D12Fence> fence) :
            fence(std::move(fence))
        {
        }

    private:
        std::atomic_uint64_t counter{ 1 };
        ComPtr<ID3D12Fence> fence;
    };
}