#pragma once
#include <Core/Container.h>
#include <Core/Mutex.h>
#include <Core/FrameManager.h>

namespace fe
{
	// Handle의 Frame Resource 대신 (FrameResource<Handle>...), Frame Resource의 Handle을 사용 할 것 (Handle<FrameResource>).
	class DeferredDeallocator final
	{
	public:
		DeferredDeallocator(const FrameManager& engineFrameManager);
		DeferredDeallocator(const DeferredDeallocator&) = delete;
		DeferredDeallocator(DeferredDeallocator&&) noexcept = delete;
		~DeferredDeallocator();

		DeferredDeallocator&  operator=(const DeferredDeallocator&) = delete;
		DeferredDeallocator& operator=(DeferredDeallocator&&) = delete;

		void RequestDeallocation(DefaultCallback&& requester);
		void FlushCurrentFrame();
		void FlushAllFrames();

	private:
		void FlushFrame(const uint8_t localFrameIdx);

	private:
		const FrameManager&															  frameManager;
		std::array<concurrency::concurrent_queue<DefaultCallback>, NumFramesInFlight> pendingRequesters;
	};
} // namespace fe