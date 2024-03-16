#include <D3D12/CommandContextPool.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandContext.h>
#include <Core/FrameManager.h>
#include <Core/Assert.h>
#include <Core/DeferredDeallocator.h>

namespace ig
{
    constexpr size_t NumExtraCommandContextsPerFrame = 4;
    CommandContextPool::CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType)
        : deferredDeallocator(deferredDeallocator),
          reservedNumCmdCtxs(std::thread::hardware_concurrency() * NumFramesInFlight + (NumExtraCommandContextsPerFrame * NumFramesInFlight))
    {
        check(reservedNumCmdCtxs > 0);
        for (size_t idx = 0; idx < reservedNumCmdCtxs; ++idx)
        {
            pool.push(new CommandContext(device.CreateCommandContext("Reserved Cmd Ctx", queueType).value()));
        }
    }

    CommandContextPool::~CommandContextPool()
    {
        check(pool.size() == reservedNumCmdCtxs);
        while (!pool.empty())
        {
            auto* cmdCtx = pool.front();
            pool.pop();
            delete cmdCtx;
        }
    }

    void CommandContextPool::Return(CommandContext* cmdContext)
    {
        ReadWriteLock lock{ mutex };
        check(cmdContext != nullptr);
        check(pool.size() < reservedNumCmdCtxs);
        pool.push(cmdContext);
    }
} // namespace ig
