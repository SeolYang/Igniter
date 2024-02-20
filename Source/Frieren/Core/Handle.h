#pragma once
#include <Core/Assert.h>
#include <Core/HashUtils.h>
#include <Core/Container.h>

namespace fe
{
	class DeferredDeallocator;
	namespace Private
	{
		void RequestDeallocation(DeferredDeallocator& deferredDeallocator, DefaultCallback&& requester);
	} // namespace Private

	class HandleManager;
	class HandleImpl
	{
		template <typename T>
		friend class WeakHandle;

		template <typename T>
		friend class UniqueHandle;

		template <typename T>
		friend class FrameHandle;

	public:
		~HandleImpl() = default;

	private:
		HandleImpl() = default;
		HandleImpl(const HandleImpl& other) = default;
		HandleImpl(HandleImpl&& other) noexcept;
		HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType);

		HandleImpl& operator=(const HandleImpl& other) = default;
		HandleImpl& operator=(HandleImpl&& other) noexcept;

		uint8_t*	   GetAddressOf(const uint64_t typeHashValue);
		const uint8_t* GetAddressOf(const uint64_t typeHashValue) const;

		bool IsValid() const { return owner != nullptr && handle != InvalidHandle; }
		bool IsAlive(const uint64_t typeHashValue) const;
		bool IsPendingDeferredDeallocation() const;

		void Deallocate(const uint64_t typeHashValue);

		void RequestDeferredDeallocation(const uint64_t typeHashVal);

	public:
		static constexpr uint64_t InvalidHandle = 0xffffffffffffffffUi64;

	private:
		HandleManager* owner = nullptr;
		uint64_t	   handle = InvalidHandle;
	};

	template <typename T>
	class WeakHandle
	{
	public:
		WeakHandle() = default;

		explicit WeakHandle(const HandleImpl newHandle)
			: handle(newHandle)
		{
		}

		WeakHandle(const WeakHandle& other) = default;
		WeakHandle(WeakHandle&& other) noexcept = default;
		~WeakHandle() = default;

		WeakHandle& operator=(const WeakHandle& other) = default;
		WeakHandle& operator=(WeakHandle&& other) noexcept = default;

		T& operator*()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		const T& operator*() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		T* operator->()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		const T* operator->() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		bool IsAlive() const { return handle.IsValid() && handle.IsAlive(EvaluatedTypeHashVal); }
		operator bool() const { return IsAlive(); }

	public:
		static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

	private:
		HandleImpl handle{};
	};

	template <typename T>
	class UniqueHandle
	{
	public:
		UniqueHandle() = default;
		UniqueHandle(const UniqueHandle&) = delete;
		UniqueHandle(UniqueHandle&& other) noexcept
			: handle(std::move(other.handle))
		{
		}

		template <typename... Args>
		UniqueHandle(HandleManager& handleManager, Args&&... args)
			: handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T))
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			new (instancePtr) T(std::forward<Args>(args)...);
		}

		~UniqueHandle()
		{
			Destroy();
		}

		UniqueHandle& operator=(const UniqueHandle&) = delete;
		UniqueHandle& operator=(UniqueHandle&& other) noexcept
		{
			if (IsAlive())
			{
				Destroy();
			}

			handle = std::move(other.handle);
			return *this;
		}

		T& operator*()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		const T& operator*() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		T* operator->()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		const T* operator->() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		bool IsAlive() const { return handle.IsValid() && handle.IsAlive(EvaluatedTypeHashVal); }
		operator bool() const { return IsAlive(); }

		void Destroy()
		{
			if (IsAlive())
			{
				T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
				if (instancePtr != nullptr)
				{
					instancePtr->~T();
					handle.Deallocate(EvaluatedTypeHashVal);
				}

				handle = {};
			}
		}

		WeakHandle<T> DeriveWeak() const
		{
			return WeakHandle<T>{ handle };
		}

	public:
		static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

	private:
		HandleImpl handle;
	};

	template <typename T>
	class FrameHandle
	{
	public:
		FrameHandle()
			: handle(), deferredDeallocator(nullptr)
		{
		}

		FrameHandle(const FrameHandle&) = delete;
		FrameHandle(FrameHandle&& other) noexcept
			: handle(std::move(other.handle)), deferredDeallocator(std::exchange(other.deferredDeallocator, nullptr))
		{
		}

		template <typename... Args>
		FrameHandle(HandleManager& handleManager, DeferredDeallocator& deferredDeallocator, Args&&... args)
			: handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T)), deferredDeallocator(&deferredDeallocator)
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);

			new (instancePtr) T(std::forward<Args>(args)...);
		}

		~FrameHandle()
		{
			Destroy();
		}

		FrameHandle& operator=(const FrameHandle&) = delete;
		FrameHandle& operator=(FrameHandle&& other) noexcept
		{
			if (IsAlive())
			{
				Destroy();
			}

			handle = std::move(other.handle);
			deferredDeallocator = std::exchange(other.deferredDeallocator, nullptr);
			return *this;
		}

		T& operator*()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		const T& operator*() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		T* operator->()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		const T* operator->() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		bool IsAlive() const { return deferredDeallocator != nullptr && handle.IsValid() && handle.IsAlive(EvaluatedTypeHashVal); }
		operator bool() const { return IsAlive(); }

		void Destroy()
		{
			if (IsAlive())
			{
				T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
				handle.RequestDeferredDeallocation(EvaluatedTypeHashVal);
				Private::RequestDeallocation(*deferredDeallocator, [instancePtr, handle = std::move(handle)]() {
					HandleImpl targetHandle = handle;
					check(targetHandle.IsPendingDeferredDeallocation());
					check(instancePtr != nullptr);
					instancePtr->~T();
					targetHandle.Deallocate(EvaluatedTypeHashVal);
				});
			}
		}

		WeakHandle<T> DeriveWeak() const
		{
			return WeakHandle<T>{ handle };
		}

	public:
		static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

	private:
		HandleImpl			 handle;
		DeferredDeallocator* deferredDeallocator = nullptr;
	};
} // namespace fe