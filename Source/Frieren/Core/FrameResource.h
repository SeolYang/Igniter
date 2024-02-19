#pragma once
#include <Core/Container.h>

// #todo Frame Resource 자체를 그냥, 하나의 컨테이너로 만들기?
// 예를들어 template <typename T> FrameResource { private: T data }.. ?
namespace fe
{
	template <typename T>
	using Deleter = std::function<void(T*)>;

	template <typename T>
	using FrameResource = std::unique_ptr<T, Deleter<T>>;

	class FrameResourceManager;
	namespace Private
	{
		using Requester = std::function<void(void)>;
		void RequestDeallocation(FrameResourceManager& frameResourceManger, Requester&& requester);
	} // namespace Private

	template <typename T, typename... Args>
	FrameResource<T> MakeFrameResourceCustom(FrameResourceManager& frameResourceManager,
											 Deleter<T>			   customDeleter,
											 Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&frameResourceManager, customDeleter](T* ptr) { Private::RequestDeallocation(frameResourceManager, [ptr, customDeleter]() { customDeleter(ptr); }); } };
	};

	template <typename T, typename... Args>
	FrameResource<T> MakeFrameResource(FrameResourceManager& frameResourceManager, Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&frameResourceManager](T* ptr) { Private::RequestDeallocation(frameResourceManager, [ptr]() { delete ptr; }); } };
	}
} // namespace fe