#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/CommandContext.h"

namespace ig
{
    class RenderDevice;
    class CommandContext;
    class CommandContextPool final
    {
    public:
        CommandContextPool(const FrameManager& frameManager, RenderDevice& renderDevice, const EQueueType cmdCtxType);
        CommandContextPool(const CommandContextPool&) = delete;
        CommandContextPool(CommandContextPool&&) noexcept = delete;
        ~CommandContextPool();

        CommandContextPool& operator=(const CommandContextPool&) = delete;
        CommandContextPool& operator=(CommandContextPool&&) noexcept = delete;

        auto Request(const String debugName)
        {
            const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
            UniqueLock poolLock{poolMutex};
            CommandContext* cmdCtx = cmdCtxs.back();
            cmdCtxs.pop_back();
            SetObjectName(&cmdCtx->GetNative(), debugName);

            const auto deleter = [this, localFrameIdx](CommandContext* ptr)
            {
                UniqueLock pendingListLock{pendingListMutex};
                pendingCmdCtxs[localFrameIdx].emplace_back(ptr);
            };

            return Ptr<CommandContext, decltype(deleter)>{cmdCtx, deleter};
        }

        void BeginFrame(const uint8_t localFrameIdx);

    private:
        constexpr static size_t NumTargetCommandContextPerThread = 8;
        const FrameManager& frameManager;
        const size_t numReservedCtx;

        Mutex poolMutex;
        eastl::vector<CommandContext*> preservedCmdCtxs;
        eastl::vector<CommandContext*> cmdCtxs;
        Mutex pendingListMutex;
        eastl::array<eastl::vector<CommandContext*>, NumFramesInFlight> pendingCmdCtxs;
    };
}    // namespace ig