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
		ForceClear();
	}

	void DeferredDeallocator::RequestDeallocation(DefaultCallback&& requester)
	{
		const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
		pendingRequesters[localFrameIdx].push(std::move(requester));
	}

	void DeferredDeallocator::BeginFrame()
	{
		BeginFrame(frameManager.GetLocalFrameIndex());
	}

	void DeferredDeallocator::BeginFrame(const uint8_t localFrameIdx)
	{
		DefaultCallback requester;
		while (pendingRequesters[localFrameIdx].try_pop(requester))
		{
			requester();
		}
		pendingRequesters[localFrameIdx].clear();
	}

	void DeferredDeallocator::ForceClear()
	{
		for (uint8_t frameIdx = 0; frameIdx < NumFramesInFlight; ++frameIdx)
		{
			BeginFrame(frameIdx);
		}
	}

} // namespace fe