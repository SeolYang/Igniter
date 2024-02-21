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
	class Handle
	{
		template <typename T>
		friend class WeakHandle;

		template <typename T, typename Destroyer>
		friend class UniqueHandle;

	public:
		Handle(const Handle& other) = default;
		Handle(Handle&& other) noexcept;
		~Handle() = default;

		Handle& operator=(const Handle& other) = default;
		Handle& operator=(Handle&& other) noexcept;

		uint8_t*	   GetAddressOf(const uint64_t typeHashValue);
		const uint8_t* GetAddressOf(const uint64_t typeHashValue) const;

		uint8_t*	   GetValidatedAddressOf(const uint64_t typeHashValue);
		const uint8_t* GetValidatedAddressOf(const uint64_t typeHashValue) const;

		bool IsValid() const { return owner != nullptr && handle != InvalidHandle; }
		bool IsAlive(const uint64_t typeHashValue) const;
		bool IsPendingDeallocation() const;

		void Deallocate(const uint64_t typeHashValue);
		void MarkAsPendingDeallocation(const uint64_t typeHashVal);

	private:
		Handle() = default;
		Handle(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType);

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

		explicit WeakHandle(const Handle newHandle)
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
		Handle handle{};
	};

	template <typename T>
	class DefaultDestroyer
	{
	public:
		void operator()(Handle handle, const uint64_t typeHashVal) const
		{
			if (handle.IsValid() && typeHashVal != InvalidHashVal)
			{
				T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(typeHashVal));
				check(instancePtr != nullptr);
				instancePtr->~T();
				handle.Deallocate(typeHashVal);
			}
		}
	};

	template <typename T, typename Destroyer = DefaultDestroyer<T>>
	class UniqueHandle
	{
	public:
		UniqueHandle() = default;
		UniqueHandle(const UniqueHandle&) = delete;
		UniqueHandle(UniqueHandle&& other) noexcept
			: handle(std::move(other.handle)), destroyer(std::move(other.destroyer))
		{
		}

		UniqueHandle(const Destroyer& destroyer)
			: handle(), destroyer(destroyer)
		{
		}

		template <typename... Args>
		UniqueHandle(HandleManager& handleManager, const Destroyer& destroyer, Args&&... args)
			: handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T)), destroyer(destroyer)
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			new (instancePtr) T(std::forward<Args>(args)...);
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
			Destroy();
			handle = std::move(other.handle);
			return *this;
		}

		T& operator*()
		{
			T* instancePtr = reinterpret_cast<T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		const T& operator*() const
		{
			const T* instancePtr = reinterpret_cast<const T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		T* operator->()
		{
			T* instancePtr = reinterpret_cast<T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		const T* operator->() const
		{
			const T* instancePtr = reinterpret_cast<const T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		bool IsAlive() const { return handle.IsAlive(EvaluatedTypeHashVal); }
		operator bool() const { return IsAlive(); }

		void Destroy()
		{
			if (IsAlive())
			{
				handle.MarkAsPendingDeallocation(EvaluatedTypeHashVal);
				destroyer(handle, EvaluatedTypeHashVal);
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
		Handle							handle;
		[[no_unique_address]] Destroyer destroyer;
	};

} // namespace fe