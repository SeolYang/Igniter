#pragma once
#include <Core/Container.h>
#include <Core/FrameManager.h>

namespace ig
{
    class DeferredDeallocator final
    {
    public:
        DeferredDeallocator(const FrameManager& engineFrameManager);
        DeferredDeallocator(const DeferredDeallocator&) = delete;
        DeferredDeallocator(DeferredDeallocator&&) noexcept = delete;
        ~DeferredDeallocator();

        DeferredDeallocator& operator=(const DeferredDeallocator&) = delete;
        DeferredDeallocator& operator=(DeferredDeallocator&&) = delete;

        void RequestDeallocation(DefaultCallback requester);
        void FlushCurrentFrame();
        void FlushAllFrames();

    private:
        void FlushFrame(const uint8_t localFrameIdx);

    private:
        const FrameManager& frameManager;
        std::array<SharedMutex, NumFramesInFlight> mutexes;
        std::array<std::queue<DefaultCallback>, NumFramesInFlight> pendingRequesterQueues;
    };

    void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester);
} // namespace ig