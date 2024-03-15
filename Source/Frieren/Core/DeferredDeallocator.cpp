#include <Core/DeferredDeallocator.h>
#include <Core/FrameManager.h>

namespace fe
{
    DeferredDeallocator::DeferredDeallocator(const FrameManager& engineFrameManager)
        : frameManager(engineFrameManager)
    {
    }

    DeferredDeallocator::~DeferredDeallocator()
    {
        FlushAllFrames();
    }

    void DeferredDeallocator::RequestDeallocation(DefaultCallback requester)
    {
        const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();

        auto& mutex = mutexes[localFrameIdx];
        auto& queue = pendingRequesterQueues[localFrameIdx];
        ReadWriteLock lock{ mutex };
        queue.push(std::move(requester));
    }

    void DeferredDeallocator::FlushCurrentFrame()
    {
        FlushFrame(frameManager.GetLocalFrameIndex());
    }

    void DeferredDeallocator::FlushFrame(const uint8_t localFrameIdx)
    {
        auto& mutex = mutexes[localFrameIdx];
        auto& queue = pendingRequesterQueues[localFrameIdx];

        ReadWriteLock lock{ mutex };
        while (!queue.empty())
        {
            queue.front()();
            queue.pop();
        }
    }

    void DeferredDeallocator::FlushAllFrames()
    {
        for (uint8_t frameIdx = 0; frameIdx < NumFramesInFlight; ++frameIdx)
        {
            FlushFrame(frameIdx);
        }
    }

    void RequestDeferredDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback requester)
    {
        deferredDeallocator.RequestDeallocation(std::move(requester));
    }
} // namespace fe