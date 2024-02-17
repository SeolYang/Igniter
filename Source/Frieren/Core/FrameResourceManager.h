#pragma once
#include <Core/Container.h>
#include <Core/Mutex.h>
#include <Core/FrameManager.h>

namespace fe
{
	class FrameResourceManager final
	{
	public:
		using Requester = std::function<void(void)>;

	public:
		FrameResourceManager(const FrameManager& engineFrameManager);
		FrameResourceManager(const FrameResourceManager&) = delete;
		FrameResourceManager(FrameResourceManager&&) noexcept = delete;
		~FrameResourceManager();

		FrameResourceManager& operator=(const FrameResourceManager&) = delete;
		FrameResourceManager& operator=(FrameResourceManager&&) = delete;

		void RequestDeallocation(Requester&& requester);
		void BeginFrame();
		void ForceClear();

	private:
		void BeginFrame(const uint8_t localFrameIdx);

	private:
		const FrameManager&														frameManager;
		std::array<RecursiveMutex, NumFramesInFlight>							mutexes;
		std::array<concurrency::concurrent_queue<Requester>, NumFramesInFlight> pendingRequesters;
	};
} // namespace fe