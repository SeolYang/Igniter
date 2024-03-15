#pragma once
#include <D3D12/Common.h>
#include <D3D12/CommandContext.h>
#include <Core/Container.h>
#include <Core/Mutex.h>

namespace fe
{
    class DeferredDeallocator;
    void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester);
} // namespace fe

namespace fe
{
    class RenderDevice;
    class CommandContextPool
    {
    public:
        CommandContextPool(DeferredDeallocator& deferredDeallocator, RenderDevice& device, const EQueueType queueType);
        ~CommandContextPool();

        auto Submit(const std::string_view debugName = "")
        {
            const static auto deleter = [this](CommandContext* ptr)
            {
                if (ptr != nullptr)
                {
                    RequestDeferredDeallocation(deferredDeallocator, [ptr, this]()
                                                { this->Return(ptr); });
                }
            };
            using ReturnType = std::unique_ptr<CommandContext, decltype(deleter)>;

            ReadWriteLock lock{ mutex };
            if (pool.empty())
            {
                return ReturnType{ nullptr, deleter };
            }

            CommandContext* cmdCtxPtr = pool.front();
            pool.pop();

            if (!debugName.empty())
            {
                SetObjectName(&cmdCtxPtr->GetNative(), debugName);
            }

            return ReturnType{ cmdCtxPtr, deleter };
        }

    private:
        void Return(CommandContext* cmdContext);

    private:
        DeferredDeallocator& deferredDeallocator;
        const size_t reservedNumCmdCtxs;

        SharedMutex mutex;
        std::queue<CommandContext*> pool;
    };
} // namespace fe