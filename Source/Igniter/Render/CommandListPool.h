#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/CommandList.h"

namespace ig
{
    class GpuDevice;
    class CommandList;

    class CommandListPool final
    {
    private:
        template <typename Deleter>
        class TempCommandList final
        {
        public:
            TempCommandList(CommandList* cmdList, Deleter&& deleter)
                : cmdList(cmdList)
                , deleter(deleter)
            {
                IG_CHECK(cmdList != nullptr);
            }

            TempCommandList(const TempCommandList&) = delete;
            TempCommandList(TempCommandList&&) noexcept = delete;
            ~TempCommandList() { deleter(cmdList); }

            TempCommandList& operator=(const TempCommandList&) = delete;
            TempCommandList& operator=(TempCommandList&&) noexcept = delete;

            [[nodiscard]] operator CommandList*() noexcept { return cmdList; }

            [[nodiscard]] CommandList* operator->() noexcept
            {
                IG_CHECK(cmdList != nullptr);
                return cmdList;
            }

            [[nodiscard]] CommandList& operator*()
            {
                IG_CHECK(cmdList != nullptr);
                return *cmdList;
            }

        private:
            Deleter deleter;
            CommandList* cmdList = nullptr;
        };

    public:
        CommandListPool(GpuDevice& gpuDevice, const EQueueType cmdListType);
        CommandListPool(const CommandListPool&) = delete;
        CommandListPool(CommandListPool&&) noexcept = delete;
        ~CommandListPool();

        CommandListPool& operator=(const CommandListPool&) = delete;
        CommandListPool& operator=(CommandListPool&&) noexcept = delete;

        auto Request(const LocalFrameIndex localFrameIdx, const String debugName)
        {
            IG_CHECK(localFrameIdx < NumFramesInFlight);

            UniqueLock poolLock{poolMutex};
            CommandList* cmdList = cmdLists.back();
            cmdLists.pop_back();
            SetObjectName(&cmdList->GetNative(), debugName);

            return TempCommandList{
                cmdList, [this, localFrameIdx](CommandList* ptr)
                {
                    UniqueLock pendingListLock{pendingListMutex};
                    pendingCmdLists[localFrameIdx].emplace_back(ptr);
                }
            };
        }

        void PreRender(const LocalFrameIndex localFrameIdx);

    private:
        constexpr static Size NumTargetCommandListPerThread = 8;
        const FrameManager& frameManager;
        const Size numReservedCtx;
        const EQueueType cmdQueueType;

        Mutex poolMutex;
        eastl::vector<CommandList*> preservedCmdLists;
        eastl::vector<CommandList*> cmdLists;
        Mutex pendingListMutex;
        eastl::array<eastl::vector<CommandList*>, NumFramesInFlight> pendingCmdLists;
    };
} // namespace ig
