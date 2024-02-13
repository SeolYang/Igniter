#include <Renderer/DeferredFrameDeallocator.h>

namespace fe
{

	DeferredFrameDeallocator::DeferredFrameDeallocator(const FrameManager& engineFrameManager)
		: frameManager(engineFrameManager)
	{
	}

	DeferredFrameDeallocator::~DeferredFrameDeallocator()
	{
		for (uint8_t frameIdx = 0; frameIdx < NumFramesInFlight; ++frameIdx)
		{
			BeginFrame(frameIdx);
		}
	}

	void DeferredFrameDeallocator::RequestDeallocation(DeleterType&& deleter)
	{
		const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
		RecursiveLock lock{ mutexes[localFrameIdx] };
		pendingDeleters[localFrameIdx].emplace_back(std::move(deleter));
	}

	void DeferredFrameDeallocator::BeginFrame()
	{
		BeginFrame(frameManager.GetLocalFrameIndex());
	}

	void DeferredFrameDeallocator::BeginFrame(const uint8_t localFrameIdx)
	{
		RecursiveLock lock{ mutexes[localFrameIdx] };
		for (auto& deleter : pendingDeleters[localFrameIdx])
		{
			deleter();
		}
		pendingDeleters[localFrameIdx].clear();
	}

} // namespace fe