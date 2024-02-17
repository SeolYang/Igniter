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
		ForceClear();
	}

	void FrameResourceManager::RequestDeallocation(Requester&& requester)
	{
		const uint8_t localFrameIdx = frameManager.GetLocalFrameIndex();
		RecursiveLock lock{ mutexes[localFrameIdx] };
		pendingRequesters[localFrameIdx].push(std::move(requester));
	}

	void FrameResourceManager::BeginFrame()
	{
		BeginFrame(frameManager.GetLocalFrameIndex());
	}

	void FrameResourceManager::BeginFrame(const uint8_t localFrameIdx)
	{
		RecursiveLock lock{ mutexes[localFrameIdx] };
		Requester	  requester;
		while (pendingRequesters[localFrameIdx].try_pop(requester))
		{
			requester();
		}
		pendingRequesters[localFrameIdx].clear();
	}

	void FrameResourceManager::ForceClear()
	{
		for (uint8_t frameIdx = 0; frameIdx < NumFramesInFlight; ++frameIdx)
		{
			BeginFrame(frameIdx);
		}
	}

} // namespace fe