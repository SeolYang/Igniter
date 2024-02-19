#pragma once
#include <Core/Container.h>
#include <Core/Mutex.h>
#include <Core/FrameManager.h>

namespace fe
{
	// Handle의 Frame Resource 대신 (FrameResource<Handle>...), Frame Resource의 Handle을 사용 할 것 (Handle<FrameResource>).
	class FrameResourceManager final
	{
	public:
		FrameResourceManager(const FrameManager& engineFrameManager);
		FrameResourceManager(const FrameResourceManager&) = delete;
		FrameResourceManager(FrameResourceManager&&) noexcept = delete;
		~FrameResourceManager();

		FrameResourceManager& operator=(const FrameResourceManager&) = delete;
		FrameResourceManager& operator=(FrameResourceManager&&) = delete;

		void RequestDeallocation(DefaultCallback&& requester);
		void BeginFrame();
		void ForceClear();

	private:
		void BeginFrame(const uint8_t localFrameIdx);

	private:
		const FrameManager&															  frameManager;
		std::array<concurrency::concurrent_queue<DefaultCallback>, NumFramesInFlight> pendingRequesters;
	};
} // namespace fe