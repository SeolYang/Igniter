#include <Igniter.h>
#include <Core/ContainerUtils.h>
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
    }

    CommandQueue::CommandQueue(CommandQueue&& other) noexcept
        : native(std::move(other.native)),
        type(other.type),
        fence(std::move(other.fence)),
        syncCounter(other.syncCounter.exchange(1))
    {
    }

    CommandQueue::~CommandQueue() {}

    GpuSync CommandQueue::MakeSync()
    {
        IG_CHECK(fence);
        const uint64_t syncPoint{ syncCounter.fetch_add(1) };
        IG_VERIFY_SUCCEEDED(native->Signal(fence.Get(), syncPoint));
        return GpuSync{ *fence.Get(), syncPoint };
    }

    void CommandQueue::ExecuteContexts(const std::span<Ref<CommandContext>> cmdCtxs)
    {
        IG_CHECK(IsValid());
        auto cmdLists{ ToVector(views::all(cmdCtxs) | views::transform([](auto& cmdCtxRef) { return &cmdCtxRef.get().GetNative(); })) };
        IG_CHECK(!cmdLists.empty());
        native->ExecuteCommandLists(static_cast<uint32_t>(cmdLists.size()), reinterpret_cast<ID3D12CommandList**>(cmdLists.data()));
    }

    void CommandQueue::SyncWith(GpuSync& sync)
    {
        IG_CHECK(native);
        ID3D12Fence& dependentQueueFence = sync.GetFence();
        IG_VERIFY_SUCCEEDED(native->Wait(&dependentQueueFence, sync.GetSyncPoint()));
    }
} // namespace ig