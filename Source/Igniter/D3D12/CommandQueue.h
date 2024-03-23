#pragma once
#include <PCH.h>
#include <D3D12/Common.h>
#include <D3D12/CommandContext.h>
#include <D3D12/GpuSync.h>
#include <Core/Container.h>

namespace ig
{
    class RenderDevice;
    class CommandContext;
    class CommandQueue final
    {
        friend class RenderDevice;

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

        void AddPendingContext(CommandContext& cmdCtx);
        GpuSync Submit();
        void SyncWith(GpuSync& sync);
        GpuSync Flush();

    private:
        CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType, ComPtr<ID3D12Fence> newFence);

        GpuSync FlushUnsafe();

    private:
        static constexpr size_t RecommendedMinNumCommandContexts = 15;

    private:
        ComPtr<ID3D12CommandQueue> native;
        const EQueueType type;

        SharedMutex mutex;
        ComPtr<ID3D12Fence> fence;
        uint64_t syncCounter = 0;
        std::vector<CommandContext::NativeType*> pendingContexts;
    };
} // namespace ig