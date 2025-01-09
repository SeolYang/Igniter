#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    class GpuDevice;
    class CommandList;
    class GpuFence;

    class CommandQueue final
    {
        friend class GpuDevice;

    public:
        CommandQueue(const CommandQueue&) = delete;
        CommandQueue(CommandQueue&& other) noexcept;
        ~CommandQueue();

        CommandQueue& operator=(const CommandQueue&) = delete;
        CommandQueue& operator=(CommandQueue&&) noexcept = delete;

        [[nodiscard]] bool IsValid() const noexcept { return native; }
        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        auto& GetNative() { return *native.Get(); }
        const auto& GetNative() const { return *native.Get(); }
        EQueueType GetType() const { return type; }

        void ExecuteCommandLists(const std::span<CommandList*> cmdLists);
        bool Signal(GpuSyncPoint& syncPoint);
        GpuSyncPoint MakeSyncPointWithSignal(GpuFence& fence);
        void Wait(GpuSyncPoint& syncPoint);

    private:
        CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType);

    private:
        static constexpr size_t RecommendedMinNumCommandLists = 16;

    private:
        ComPtr<ID3D12CommandQueue> native;
        const EQueueType type;
    };
} // namespace ig
