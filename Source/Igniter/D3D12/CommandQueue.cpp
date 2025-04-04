#include "Igniter/Igniter.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/CommandList.h"

namespace ig
{
    CommandQueue::CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, GpuFence fence, const EQueueType specifiedType)
        : type(specifiedType)
        , native(std::move(newNativeQueue))
        , fence(std::move(fence))
    {}

    CommandQueue::CommandQueue(CommandQueue&& other) noexcept
        : type(other.type)
        , native(std::move(other.native))
        , fence(std::move(other.fence))
    {}

    CommandQueue::~CommandQueue() {}

    void CommandQueue::ExecuteCommandLists(const std::span<CommandList*> cmdLists)
    {
        IG_CHECK(IsValid());
        IG_CHECK(!cmdLists.empty());

        auto toNative = views::all(cmdLists) |
            views::filter([](CommandList* cmdList)
            {
                return cmdList != nullptr;
            }) |
            views::transform([](CommandList* cmdList)
            {
                return &cmdList->GetNative();
            });

        Vector<CommandList::NativeType*> natives = ToVector(toNative);
        native->ExecuteCommandLists(static_cast<U32>(natives.size()), reinterpret_cast<ID3D12CommandList**>(natives.data()));
    }

    void CommandQueue::ExecuteCommandList(CommandList& cmdList)
    {
        CommandList* cmdLists[]{&cmdList};
        ExecuteCommandLists(cmdLists);
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

    GpuSyncPoint CommandQueue::MakeSyncPointWithSignal(GpuFence& externalFence)
    {
        IG_CHECK(native);
        IG_CHECK(externalFence);
        GpuSyncPoint syncPoint{externalFence.MakeSyncPoint()};
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
        if (syncPoint.IsValid())
        {
            ID3D12Fence& nativeFence = syncPoint.GetFence();
            IG_VERIFY_SUCCEEDED(native->Wait(&nativeFence, syncPoint.GetSyncPoint()));
        }
    }
} // namespace ig
