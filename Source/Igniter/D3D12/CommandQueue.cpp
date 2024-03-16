#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <D3D12/RenderDevice.h>

namespace ig
{
    CommandQueue::CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType, ComPtr<ID3D12Fence> newFence)
        : native(std::move(newNativeQueue)),
          type(specifiedType),
          fence(std::move(newFence))
    {
        pendingContexts.reserve(RecommendedMinNumCommandContexts);
    }

    CommandQueue::CommandQueue(CommandQueue&& other) noexcept
        : native(std::move(other.native)),
          type(other.type),
          fence(std::move(other.fence)),
          syncCounter(std::exchange(other.syncCounter, 0)),
          pendingContexts(std::move(other.pendingContexts))
    {
    }

    CommandQueue::~CommandQueue() {}

    GpuSync CommandQueue::FlushUnsafe()
    {
        ++syncCounter;
        verify_succeeded(native->Signal(fence.Get(), syncCounter));
        return { *fence.Get(), syncCounter };
    }

    GpuSync CommandQueue::Flush()
    {
        ReadWriteLock lock{ mutex };
        return FlushUnsafe();
    }

    void CommandQueue::AddPendingContext(CommandContext& cmdCtx)
    {
        check(cmdCtx);
        ReadWriteLock lock{ mutex };
        pendingContexts.emplace_back(cmdCtx);
    }

    GpuSync CommandQueue::Submit()
    {
        check(IsValid());
        check(fence);

        ReadWriteLock lock{ mutex };
        check(!pendingContexts.empty());
        auto pendingNativeCmdLists = TransformToNative(std::span{ pendingContexts });
        native->ExecuteCommandLists(static_cast<uint32_t>(pendingNativeCmdLists.size()),
                                    reinterpret_cast<ID3D12CommandList* const*>(pendingNativeCmdLists.data()));
        pendingContexts.clear();

        return FlushUnsafe();
    }

    void CommandQueue::SyncWith(GpuSync& sync)
    {
        check(native);
        ID3D12Fence& dependentQueueFence = sync.GetFence();
        verify_succeeded(native->Wait(&dependentQueueFence, sync.GetSyncPoint()));
    }
} // namespace ig