#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/CommandListPool.h"

namespace ig
{
    CommandListPool::CommandListPool(GpuDevice& gpuDevice, const EQueueType cmdListType) :
        frameManager(frameManager),
        numReservedCtx(NumTargetCommandListPerThread * std::thread::hardware_concurrency() * NumFramesInFlight),
        cmdQueueType(cmdListType)
    {
        IG_CHECK(numReservedCtx > 0);
        cmdLists.reserve(numReservedCtx);
        preservedCmdLists.reserve(numReservedCtx);
        for (const auto idx : views::iota(0Ui64, numReservedCtx))
        {
            cmdLists.push_back(new CommandList(gpuDevice.CreateCommandList(std::format("{}_Pool-{}", cmdListType, idx), cmdListType).value()));
            preservedCmdLists.push_back(cmdLists.back());
        }
    }

    CommandListPool::~CommandListPool()
    {
        for (const uint8_t localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            PreRender(localFrameIdx);
        }

        IG_CHECK(preservedCmdLists.size() == cmdLists.size());
        /* 외부로 나간 Command Context 포인터가 반환 되지 않아서 메모리 릭이 발생하는 경우를 방지 하기 위함 */
        for (CommandList* cmdList : preservedCmdLists)
        {
            delete cmdList;
        }
    }

    void CommandListPool::PreRender(const LocalFrameIndex localFrameIdx)
    {
        ScopedLock lock{poolMutex, pendingListMutex};
        for (CommandList* cmdList : pendingCmdLists[localFrameIdx])
        {
            cmdLists.emplace_back(cmdList);
        }
        pendingCmdLists[localFrameIdx].clear();
    }
} // namespace ig
