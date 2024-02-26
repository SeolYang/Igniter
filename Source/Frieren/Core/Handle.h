#pragma once
#include <Core/Assert.h>
#include <Core/HashUtils.h>
#include <Core/MemUtils.h>
#include <Core/Container.h>

namespace fe
{
	template <typename T, typename Finalizer>
	class WeakHandle;
	template <typename T, typename Destroyer>
	class Handle;

	class HandleManager;
	namespace details
	{
		class HandleImpl
		{
			template <typename T, typename Finalizer>
			friend class WeakHandle;

			template <typename T, typename Destroyer>
			friend class Handle;

		public:
			HandleImpl(const HandleImpl& other) = default;
			HandleImpl(HandleImpl&& other) noexcept;
			~HandleImpl() = default;

			HandleImpl& operator=(const HandleImpl& other) = default;
			HandleImpl& operator=(HandleImpl&& other) noexcept;

			uint8_t*	   GetAddressOf(const uint64_t typeHashValue);
			const uint8_t* GetAddressOf(const uint64_t typeHashValue) const;

			uint8_t*	   GetValidatedAddressOf(const uint64_t typeHashValue);
			const uint8_t* GetValidatedAddressOf(const uint64_t typeHashValue) const;

			bool IsValid() const { return owner != nullptr && handle != InvalidHandle; }
			bool IsAlive(const uint64_t typeHashValue) const;
			bool IsPendingDeallocation() const;

			void Deallocate(const uint64_t typeHashValue);
			void MarkAsPendingDeallocation(const uint64_t typeHashVal);

			size_t GetHash() const { return handle; }

		private:
			HandleImpl() = default;
			HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType);

		public:
			static constexpr uint64_t InvalidHandle = 0xffffffffffffffffUi64;

		private:
			HandleManager* owner = nullptr;
			uint64_t	   handle = InvalidHandle;
		};
	} // namespace details

	template <typename T>
	class DefaultFinalizer
	{
	public:
		void operator()(const WeakHandle<T, DefaultFinalizer>&, T*)
		{
			/* Empty */
		}
	};

	template <typename T, typename Finalizer = DefaultFinalizer<T>>
	class WeakHandle
	{
	public:
		WeakHandle() = default;

		explicit WeakHandle(const details::HandleImpl newHandle)
			: handle(newHandle)
		{
			check(IsAlive());
			if constexpr (std::is_pointer_v<Finalizer>)
			{
				finalizer = nullptr;
			}
		}

		template <typename Fn>
			requires(std::is_same_v<std::decay_t<Fn>, Finalizer>)
		explicit WeakHandle(const details::HandleImpl& newHandle, Fn&& finalizer)
			: handle(newHandle),
			  finalizer(std::forward<Fn>(finalizer))
		{
			check(IsAlive());
		}

		WeakHandle(const WeakHandle& other) = default;
		WeakHandle(WeakHandle&& other) noexcept
			: handle(std::move(other.handle)),
			  finalizer(std::move(other.finalizer))
		{
			if constexpr (std::is_pointer_v<Finalizer>)
			{
				this->finalizer = nullptr;
			}
		}

		~WeakHandle()
		{
			if (IsAlive())
			{
				if constexpr (std::is_pointer_v<Finalizer>)
				{
					if (finalizer != nullptr)
					{
						(*finalizer)(*this, reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal)));
					}
				}
				else
				{
					finalizer(*this, reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal)));
				}
			}
		}

		WeakHandle& operator=(const WeakHandle& other) = default;
		WeakHandle& operator=(WeakHandle&& other) noexcept = default;

		[[nodiscard]] T& operator*()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		[[nodiscard]] const T& operator*() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		[[nodiscard]] T* operator->()
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		[[nodiscard]] const T* operator->() const
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		[[nodiscard]] bool IsAlive() const { return handle.IsValid() && handle.IsAlive(EvaluatedTypeHashVal); }
		[[nodiscard]] operator bool() const { return IsAlive(); }

		[[nodiscard]] size_t GetHash() const { return handle.GetHash(); }

	public:
		static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

	private:
		details::HandleImpl				handle{};
		[[no_unique_address]] Finalizer finalizer;
	};

	template <typename T>
	class DefaultDestroyer
	{
	public:
		void operator()(details::HandleImpl handle, const uint64_t typeHashVal, T* instance) const
		{
			check(instance != nullptr);
			if (handle.IsValid() && typeHashVal != InvalidHashVal)
			{
				if (instance != nullptr)
				{
					instance->~T();
				}
				handle.Deallocate(typeHashVal);
			}
		}
	};

	template <typename T, typename Destroyer = DefaultDestroyer<T>>
	class Handle
	{
	public:
		Handle()
		{
			if constexpr (std::is_pointer_v<Destroyer>)
			{
				this->destroyer = nullptr;
			}
		}

		Handle(const Handle&) = delete;

		Handle(Handle&& other) noexcept
			: handle(std::move(other.handle)),
			  destroyer(std::move(other.destroyer))
		{
			if constexpr (std::is_pointer_v<Destroyer>)
			{
				other.destroyer = nullptr;
			}
		}

		template <typename Dx>
		Handle(Dx&& destroyer)
			: destroyer(std::forward<Dx>(destroyer))
		{
		}

		template <typename... Args, typename Dx>
			requires(std::is_same_v<std::decay_t<Dx>, Destroyer>)
		Handle(HandleManager& handleManager, Dx&& destroyer, Args&&... args)
			: handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T)),
			  destroyer(std::forward<Dx>(destroyer))
		{
			check(IsAlive());
			if constexpr (std::is_pointer_v<Destroyer>)
			{
				check(destroyer != nullptr);
			}

			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			new (instancePtr) T(std::forward<Args>(args)...);
		}

		template <typename... Args>
			requires(!std::is_pointer_v<Destroyer> && std::is_constructible_v<Destroyer>)
		Handle(HandleManager& handleManager, Args&&... args)
			: handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T))
		{
			check(IsAlive());
			T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			new (instancePtr) T(std::forward<Args>(args)...);
		}

		~Handle()
		{
			Destroy();
		}

		Handle& operator=(const Handle&) = delete;

		template <typename... Args, typename D = Destroyer>
			requires(!std::is_pointer_v<D>)
		Handle& operator=(Handle&& other) noexcept
		{
			Destroy();
			handle = std::move(other.handle);
			destroyer = std::move(other.destroyer);
			return *this;
		}

		template <typename... Args>
			requires(std::is_pointer_v<Destroyer>)
		Handle& operator=(Handle&& other) noexcept
		{
			Destroy();
			handle = std::move(other.handle);
			destroyer = std::exchange(other.destroyer, nullptr);
			return *this;
		}

		[[nodiscard]] T& operator*()
		{
			T* instancePtr = reinterpret_cast<T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		[[nodiscard]] const T& operator*() const
		{
			const T* instancePtr = reinterpret_cast<const T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return *instancePtr;
		}

		[[nodiscard]] T* operator->()
		{
			T* instancePtr = reinterpret_cast<T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		[[nodiscard]] const T* operator->() const
		{
			const T* instancePtr = reinterpret_cast<const T*>(handle.GetValidatedAddressOf(EvaluatedTypeHashVal));
			check(instancePtr != nullptr);
			return instancePtr;
		}

		[[nodiscard]] bool IsAlive() const { return handle.IsAlive(EvaluatedTypeHashVal); }
		[[nodiscard]] operator bool() const { return IsAlive(); }

		void Destroy()
		{
			if (IsAlive())
			{
				handle.MarkAsPendingDeallocation(EvaluatedTypeHashVal);
				T* instance = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
				check(instance != nullptr);
				if constexpr (std::is_pointer_v<Destroyer>)
				{
					check(destroyer != nullptr);
					(*destroyer)(handle, EvaluatedTypeHashVal, instance);
				}
				else
				{
					destroyer(handle, EvaluatedTypeHashVal, instance);
				}
				handle = {};
			}
		}

		[[nodiscard]] WeakHandle<T> DeriveWeak() const
		{
			return WeakHandle<T>{ handle };
		}

		template <typename Finalizer>
		[[nodiscard]] WeakHandle<T, Finalizer> DeriveWeak(Finalizer&& finalizer) const
		{
			return WeakHandle<T, Finalizer>{ handle, finalizer };
		}

		[[nodiscard]] size_t GetHash() const { return handle.GetHash(); }

	public:
		static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

	private:
		details::HandleImpl				handle;
		[[no_unique_address]] Destroyer destroyer;
	};
} // namespace fe

namespace std
{
	template <typename T, typename Dx>
	class hash<fe::Handle<T, Dx>>
	{
	public:
		size_t operator()(const fe::Handle<T, Dx>& handle) const { return handle.GetHash(); }
	};

	template <typename T>
	class hash<fe::WeakHandle<T>>
	{
	public:
		size_t operator()(const fe::WeakHandle<T>& handle) const { return handle.GetHash(); }
	};
} // namespace std