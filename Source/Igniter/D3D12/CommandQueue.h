#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

namespace ig
{
    class GpuDevice;
    class CommandContext;

    class CommandQueue final
    {
        friend class GpuDevice;

    public:
        CommandQueue(const CommandQueue&) = delete;
        CommandQueue(CommandQueue&& other) noexcept;
        ~CommandQueue();

        CommandQueue& operator=(const CommandQueue&) = delete;
        CommandQueue& operator=(CommandQueue&&) noexcept = delete;

        bool IsValid() const { return native; }
        operator bool() const { return IsValid(); }

        auto& GetNative() { return *native.Get(); }
        EQueueType GetType() const { return type; }

        void ExecuteContexts(const std::span<CommandContext*> cmdCtxs);
        GpuSyncPoint MakeSync();
        void SyncWith(GpuSyncPoint& sync);

    private:
        CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType, ComPtr<ID3D12Fence> newFence);

    private:
        static constexpr size_t RecommendedMinNumCommandContexts = 16;

    private:
        ComPtr<ID3D12CommandQueue> native;
        const EQueueType type;

        /*Mutex mutex;*/
        ComPtr<ID3D12Fence> fence;
        std::atomic_uint64_t syncCounter{ 1 };
    };
}    // namespace ig
