#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    class GpuDevice;
    class CommandList;

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
        void ExecuteCommandList(CommandList& cmdList);
        bool Signal(GpuSyncPoint& syncPoint);
        GpuSyncPoint MakeSyncPointWithSignal() { return MakeSyncPointWithSignal(fence); }
        void Wait(GpuSyncPoint& syncPoint);

    private:
        CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, GpuFence fence, const EQueueType specifiedType);
        /* @note 추후 별도의 외부 Fence를 사용할 상황을 대비해 남겨둠. */
        GpuSyncPoint MakeSyncPointWithSignal(GpuFence& externalFence);

    private:
        static constexpr Size RecommendedMinNumCommandLists = 16;

    private:
        EQueueType type;
        ComPtr<ID3D12CommandQueue> native;
        GpuFence fence;
    };
} // namespace ig
