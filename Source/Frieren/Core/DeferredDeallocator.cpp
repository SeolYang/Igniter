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
		pendingRequesters[localFrameIdx].push(std::move(requester));
	}

	void DeferredDeallocator::FlushCurrentFrame()
	{
		FlushFrame(frameManager.GetLocalFrameIndex());
	}

	void DeferredDeallocator::FlushFrame(const uint8_t localFrameIdx)
	{
		DefaultCallback requester;
		while (pendingRequesters[localFrameIdx].try_pop(requester))
		{
			requester();
		}
		pendingRequesters[localFrameIdx].clear();
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