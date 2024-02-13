#include <Core/FrameResourceManager.h>

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

	void FrameResourceManager::RequestDeallocation(DeleterType&& deleter)
	{
		const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
		RecursiveLock lock{ mutexes[localFrameIdx] };
		pendingDeleters[localFrameIdx].emplace_back(std::move(deleter));
	}

	void FrameResourceManager::BeginFrame()
	{
		BeginFrame(frameManager.GetLocalFrameIndex());
	}

	void FrameResourceManager::BeginFrame(const uint8_t localFrameIdx)
	{
		RecursiveLock lock{ mutexes[localFrameIdx] };
		for (auto& deleter : pendingDeleters[localFrameIdx])
		{
			deleter();
		}
		pendingDeleters[localFrameIdx].clear();
	}

} // namespace fe