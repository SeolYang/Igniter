#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/CommandContext.h"

namespace ig
{
    class GpuDevice;
    class CommandContext;
    class CommandContextPool final
    {
    private:
        template <typename Deleter>
        class TempCommandContext final
        {
        public:
            TempCommandContext(CommandContext* cmdCtx, Deleter&& deleter) : cmdCtx(cmdCtx), deleter(deleter) { IG_CHECK(cmdCtx != nullptr); }
            TempCommandContext(const TempCommandContext&) = delete;
            TempCommandContext(TempCommandContext&&) noexcept = delete;
            ~TempCommandContext() { deleter(cmdCtx); }

            TempCommandContext& operator=(const TempCommandContext&) = delete;
            TempCommandContext& operator=(TempCommandContext&&) noexcept = delete;

            [[nodiscard]] explicit operator CommandContext* () noexcept { return cmdCtx; }

            [[nodiscard]] CommandContext* operator->() noexcept
            {
                IG_CHECK(cmdCtx != nullptr);
                return cmdCtx;
            }

            [[nodiscard]] CommandContext& operator*() noexcept
            {
                IG_CHECK(cmdCtx != nullptr);
                return *cmdCtx;
            }

        private:
            Deleter deleter;
            CommandContext* cmdCtx = nullptr;
        };

    public:
        CommandContextPool(GpuDevice& gpuDevice, const EQueueType cmdCtxType);
        CommandContextPool(const CommandContextPool&) = delete;
        CommandContextPool(CommandContextPool&&) noexcept = delete;
        ~CommandContextPool();

        CommandContextPool& operator=(const CommandContextPool&) = delete;
        CommandContextPool& operator=(CommandContextPool&&) noexcept = delete;

        auto Request(const LocalFrameIndex localFrameIdx, const String debugName)
        {
            IG_CHECK(localFrameIdx < NumFramesInFlight);

            UniqueLock poolLock{ poolMutex };
            CommandContext* cmdCtx = cmdCtxs.back();
            cmdCtxs.pop_back();
            SetObjectName(&cmdCtx->GetNative(), debugName);

            return TempCommandContext{ cmdCtx, [this, localFrameIdx](CommandContext* ptr)
                {
                    UniqueLock pendingListLock{pendingListMutex};
                    pendingCmdCtxs[localFrameIdx].emplace_back(ptr);
                } };
        }

        void PreRender(const LocalFrameIndex localFrameIdx);

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