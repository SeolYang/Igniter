#pragma once
#include <Igniter.h>

namespace ig
{
    class FrameManager;

    class DeferredDeallocator final
    {
    public:
        DeferredDeallocator(const FrameManager& frameManager);
        DeferredDeallocator(const DeferredDeallocator&)     = delete;
        DeferredDeallocator(DeferredDeallocator&&) noexcept = delete;
        ~DeferredDeallocator();

        DeferredDeallocator& operator=(const DeferredDeallocator&) = delete;
        DeferredDeallocator& operator=(DeferredDeallocator&&)      = delete;

        void RequestDeallocation(std::function<void()>&& requester);
        void FlushCurrentFrame();
        void FlushAllFrames();

    private:
        void FlushFrame(const uint8_t localFrameIdx);

    private:
        const FrameManager&                                              frameManager;
        std::array<SharedMutex, NumFramesInFlight>                       mutexes;
        std::array<std::queue<std::function<void()>>, NumFramesInFlight> pendingRequesterQueues;
    };

    void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, std::function<void()>&& requester);
} // namespace ig
