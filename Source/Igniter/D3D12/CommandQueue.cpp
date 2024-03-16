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
        IG_VERIFY_SUCCEEDED(native->Signal(fence.Get(), syncCounter));
        return { *fence.Get(), syncCounter };
    }

    GpuSync CommandQueue::Flush()
    {
        ReadWriteLock lock{ mutex };
        return FlushUnsafe();
    }

    void CommandQueue::AddPendingContext(CommandContext& cmdCtx)
    {
        if (!cmdCtx)
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        ReadWriteLock lock{ mutex };
        pendingContexts.emplace_back(&cmdCtx.GetNative());
    }

    GpuSync CommandQueue::Submit()
    {
        IG_CHECK(IsValid());
        IG_CHECK(fence);

        ReadWriteLock lock{ mutex };
        IG_CHECK(!pendingContexts.empty());
        native->ExecuteCommandLists(static_cast<uint32_t>(pendingContexts.size()),
                                    reinterpret_cast<ID3D12CommandList* const*>(pendingContexts.data()));
        pendingContexts.clear();

        return FlushUnsafe();
    }

    void CommandQueue::SyncWith(GpuSync& sync)
    {
        IG_CHECK(native);
        ID3D12Fence& dependentQueueFence = sync.GetFence();
        IG_VERIFY_SUCCEEDED(native->Wait(&dependentQueueFence, sync.GetSyncPoint()));
    }
} // namespace ig