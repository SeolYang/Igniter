#include <Igniter.h>
#include <Core/FrameManager.h>
#include <Core/Assert.h>
#include <Core/DeferredDeallocator.h>
#include <D3D12/CommandContextPool.h>
#include <D3D12/CommandContext.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/CommandContext.h>

namespace ig
{
    constexpr size_t NumExtraCommandContextsPerFrame = 4;
    CommandContextPool::CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType)
        : deferredDeallocator(deferredDeallocator),
          reservedNumCmdCtxs(std::thread::hardware_concurrency() * NumFramesInFlight + (NumExtraCommandContextsPerFrame * NumFramesInFlight))
    {
        IG_CHECK(reservedNumCmdCtxs > 0);
        for (size_t idx = 0; idx < reservedNumCmdCtxs; ++idx)
        {
            pool.push(new CommandContext(device.CreateCommandContext("Reserved Cmd Ctx", queueType).value()));
        }
    }

    CommandContextPool::~CommandContextPool()
    {
        IG_CHECK(pool.size() == reservedNumCmdCtxs);
        while (!pool.empty())
        {
            auto* cmdCtx = pool.front();
            pool.pop();
            delete cmdCtx;
        }
    }

    std::unique_ptr<CommandContext, std::function<void(CommandContext*)>> CommandContextPool::Request(const std::string_view debugName)
    {
        const auto deleter = [this](CommandContext* ptr)
        {
            if (ptr != nullptr)
            {
                RequestDeferredDeallocation(deferredDeallocator, [ptr, this]()
                                            { this->Return(ptr); });
            }
        };

        ReadWriteLock lock{ mutex };
        CommandContext* cmdCtxPtr = nullptr;
        if (pool.empty())
        {
            return nullptr;
        }
        else
        {
            cmdCtxPtr = pool.front();
            pool.pop();

            if (!debugName.empty())
            {
                SetObjectName(&cmdCtxPtr->GetNative(), debugName);
            }
        }

        return { cmdCtxPtr, deleter };
    }

    void CommandContextPool::Return(CommandContext* cmdContext)
    {
        ReadWriteLock lock{ mutex };
        IG_CHECK(cmdContext != nullptr);
        IG_CHECK(pool.size() < reservedNumCmdCtxs);
        pool.push(cmdContext);
    }
} // namespace ig
