#pragma once
#include <Core/Container.h>
#include <Core/Mutex.h>
#include <Core/FrameManager.h>

namespace fe
{
	class Renderer;
	class FrameResourceManager final
	{
	public:
		using DeleterType = std::function<void(void)>;

	public:
		FrameResourceManager(const FrameManager& engineFrameManager);
		FrameResourceManager(const FrameResourceManager&) = delete;
		FrameResourceManager(FrameResourceManager&&) noexcept = delete;
		~FrameResourceManager();

		FrameResourceManager& operator=(const FrameResourceManager&) = delete;
		FrameResourceManager& operator=(FrameResourceManager&&) = delete;

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
	FrameResource<T> MakeFrameResource(FrameResourceManager&   frameResourceManager,
									   std::function<void(T*)> deleter,
									   Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&frameResourceManager, deleter](T* ptr) { frameResourceManager.RequestDeallocation([ptr, deleter]() { deleter(ptr); }); } };
	};
} // namespace fe