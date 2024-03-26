#pragma once
#include <Igniter.h>
#include <D3D12/Common.h>

namespace ig
{
    class DeferredDeallocator;
    void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, std::function<void()> requester);
} // namespace ig

namespace ig
{
    class RenderDevice;
    class CommandContext;
    class CommandContextPool final
    {
    public:
        CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType);
        ~CommandContextPool();

        std::unique_ptr<CommandContext, std::function<void(CommandContext*)>> Submit(const std::string_view debugName = "");

    private:
        void Return(CommandContext* cmdContext);

    private:
        DeferredDeallocator& deferredDeallocator;
        const size_t reservedNumCmdCtxs;

        SharedMutex mutex;
        std::queue<CommandContext*> pool;
    };
} // namespace ig