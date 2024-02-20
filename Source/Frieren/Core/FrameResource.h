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

	class DeferredDeallocator;
	namespace Private
	{
		void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester);
	} // namespace Private

	template <typename T, typename... Args>
	FrameResource<T> MakeFrameResourceCustom(DeferredDeallocator& deferredDeallocator,
											 Deleter<T>			   customDeleter,
											 Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&deferredDeallocator, customDeleter](T* ptr) { Private::RequestDeallocation(deferredDeallocator, [ptr, customDeleter]() { customDeleter(ptr); }); } };
	};

	template <typename T, typename... Args>
	FrameResource<T> MakeFrameResource(DeferredDeallocator& deferredDeallocator, Args&&... args)
	{
		return FrameResource<T>{ new T(std::forward<Args>(args)...),
								 [&deferredDeallocator](T* ptr) { Private::RequestDeallocation(deferredDeallocator, [ptr]() { delete ptr; }); } };
	}
} // namespace fe