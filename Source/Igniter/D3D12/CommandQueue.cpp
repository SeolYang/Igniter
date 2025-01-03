#include "Igniter/Igniter.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/CommandContext.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuFence.h"

namespace ig
{
    CommandQueue::CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType) :
        native(std::move(newNativeQueue)),
        type(specifiedType) {}

    CommandQueue::CommandQueue(CommandQueue&& other) noexcept :
        native(std::move(other.native)),
        type(other.type) {}

    CommandQueue::~CommandQueue() {}

    void CommandQueue::ExecuteContexts(const std::span<CommandContext*> cmdCtxs)
    {
        IG_CHECK(IsValid());
        auto toNative = views::all(cmdCtxs) | views::filter([](CommandContext* cmdCtx)
                                                            { return cmdCtx != nullptr; }) |
                views::transform([](CommandContext* cmdCtx)
                                 { return &cmdCtx->GetNative(); });
        eastl::vector<CommandContext::NativeType*> natives = ToVector(toNative);
        native->ExecuteCommandLists(static_cast<uint32_t>(natives.size()), reinterpret_cast<ID3D12CommandList**>(natives.data()));
    }

    bool CommandQueue::Signal(GpuSyncPoint& syncPoint)
    {
        IG_CHECK(native);
        IG_CHECK(syncPoint);
        IG_CHECK(!syncPoint.IsExpired());
        ID3D12Fence& nativeFence = syncPoint.GetFence();
        if (FAILED(native->Signal(&nativeFence, syncPoint.GetSyncPoint())))
        {
            return false;
        }

        return true;
    }

    GpuSyncPoint CommandQueue::MakeSyncPointWithSignal(GpuFence& fence)
    {
        IG_CHECK(native);
        IG_CHECK(fence);
        GpuSyncPoint syncPoint{fence.MakeSyncPoint()};
        if (!syncPoint.IsValid())
        {
            return GpuSyncPoint::Invalid();
        }

        if (!Signal(syncPoint))
        {
            return GpuSyncPoint::Invalid();
        }

        return syncPoint;
    }

    void CommandQueue::Wait(GpuSyncPoint& syncPoint)
    {
        IG_CHECK(native);
        ID3D12Fence& nativeFence = syncPoint.GetFence();
        IG_VERIFY_SUCCEEDED(native->Wait(&nativeFence, syncPoint.GetSyncPoint()));
    }
} // namespace ig
