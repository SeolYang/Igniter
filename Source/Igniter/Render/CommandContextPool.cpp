#include <Igniter.h>
#include <D3D12/RenderDevice.h>
#include <Render/CommandContextPool.h>

namespace ig
{
    CommandContextPool::CommandContextPool(const FrameManager& frameManager, RenderDevice& renderDevice, const EQueueType cmdCtxType)
        : frameManager(frameManager), numReservedCtx(NumTargetCommandContextPerThread * std::thread::hardware_concurrency() * NumFramesInFlight)
    {
        IG_CHECK(numReservedCtx > 0);
        cmdCtxs.reserve(numReservedCtx);
        preservedCmdCtxs.reserve(numReservedCtx);
        for (const auto idx : views::iota(0Ui64, numReservedCtx))
        {
            cmdCtxs.push_back(new CommandContext(renderDevice.CreateCommandContext(std::format("{}_Pool-{}", cmdCtxType, idx), cmdCtxType).value()));
            preservedCmdCtxs.push_back(cmdCtxs.back());
        }
    }

    CommandContextPool::~CommandContextPool()
    {
        for (const uint8_t localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            BeginFrame(localFrameIdx);
        }

        IG_CHECK(preservedCmdCtxs.size() == cmdCtxs.size());
        /* 외부로 나간 Command Context 포인터가 반환 되지 않아서 메모리 릭이 발생하는 경우를 방지 하기 위함 */
        for (CommandContext* cmdCtx : preservedCmdCtxs)
        {
            delete cmdCtx;
        }
    }

    void CommandContextPool::BeginFrame(const uint8_t localFrameIdx)
    {
        ScopedLock lock{poolMutex, pendingListMutex};
        for (CommandContext* cmdCtx : pendingCmdCtxs[localFrameIdx])
        {
            cmdCtxs.emplace_back(cmdCtx);
        }
        pendingCmdCtxs[localFrameIdx].clear();
    }
}    // namespace ig