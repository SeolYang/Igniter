#include <Core/FrameResourceManager.h>
#include <Core/FrameManager.h>

namespace fe
{
	FrameResourceManager::FrameResourceManager(const FrameManager& engineFrameManager)
		: frameManager(engineFrameManager)
	{
	}

	FrameResourceManager::~FrameResourceManager()
	{
		for (uint8_t frameIdx = 0; frameIdx < NumFramesInFlight; ++frameIdx)
		{
			BeginFrame(frameIdx);
		}
	}

	void FrameResourceManager::RequestDeallocation(Requester&& requester)
	{
		const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
		RecursiveLock lock{ mutexes[localFrameIdx] };
		pendingRequesters[localFrameIdx].emplace_back(std::move(requester));
	}

	void FrameResourceManager::BeginFrame()
	{
		BeginFrame(frameManager.GetLocalFrameIndex());
	}

	void FrameResourceManager::BeginFrame(const uint8_t localFrameIdx)
	{
		RecursiveLock lock{ mutexes[localFrameIdx] };
		for (auto& deleter : pendingRequesters[localFrameIdx])
		{
			deleter();
		}
		pendingRequesters[localFrameIdx].clear();
	}

} // namespace fe