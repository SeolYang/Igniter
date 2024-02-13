#pragma once
#include <Renderer/Common.h>
#include <Core/Mutex.h>

namespace fe
{
	class Renderer;
	class DeferredFrameDeallocator final
	{
	public:
		using DeleterType = std::function<void(void)>;

	public:
		DeferredFrameDeallocator(const FrameManager& engineFrameManager);
		DeferredFrameDeallocator(const DeferredFrameDeallocator&) = delete;
		DeferredFrameDeallocator(DeferredFrameDeallocator&&) noexcept = delete;
		~DeferredFrameDeallocator();

		DeferredFrameDeallocator& operator=(const DeferredFrameDeallocator&) = delete;
		DeferredFrameDeallocator& operator=(DeferredFrameDeallocator&&) = delete;

		void RequestDeallocation(DeleterType&& deleter);
		void BeginFrame();

	private:
		void BeginFrame(const uint8_t localFrameIdx);

	private:
		const FrameManager&										frameManager;
		std::array<RecursiveMutex, NumFramesInFlight>			mutexes;
		std::array<std::vector<DeleterType>, NumFramesInFlight> pendingDeleters;
	};

	template <typename T>
	using FrameResource = std::unique_ptr<T, void(void)>;

	template <typename T, typename... Args>
	FrameResource<T> MakeFrameResource(DeferredFrameDeallocator&			   deallocator,
									   DeferredFrameDeallocator::DeleterType&& deleter,
									   Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&deallocator, internalDeleter = std::move(deleter)]()
								 {
									 deallocator.RequestDeallocation(std::move(internalDeleter));
								 } };
	};
} // namespace fe