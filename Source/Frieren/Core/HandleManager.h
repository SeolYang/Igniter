#pragma once
#include <Core/String.h>
#include <Core/Pool.h>
#include <Core/Mutex.h>
#include <Core/HashUtils.h>
#include <Core/Misc.h>

namespace fe
{
	class HandleManager;
	namespace Private
	{
		class HandleRepository
		{
		public:
			virtual ~HandleRepository() = default;

		protected:
			HandleRepository() = default;
		};

		template <typename T>
		class GenericHandleRepository final : public HandleRepository
		{
			friend class fe::HandleManager;

		public:
			GenericHandleRepository()
				: HandleRepository() {}

			virtual ~GenericHandleRepository()
			{
				const bool bNoRemainHandles = nameMap.empty();
				verify(bNoRemainHandles);
				for (const auto namePair : nameMap)
				{
					Deallocation(namePair.first);
				}
			}

			GenericHandleRepository(const GenericHandleRepository&) = delete;
			GenericHandleRepository(GenericHandleRepository&&) noexcept = delete;

			GenericHandleRepository& operator=(const GenericHandleRepository&) = delete;
			GenericHandleRepository& operator=(GenericHandleRepository&&) = delete;

		private:
			static uint64_t PackAllocationToIndex(const PoolAllocation<T> allocation)
			{
				constexpr uint64_t MaximumIndexRange = 0x00000000ffffffff;
				verify(allocation.ChunkIndex < MaximumIndexRange || allocation.ElementIndex < MaximumIndexRange);
				uint64_t result = 0;
				result = ((allocation.ChunkIndex & MaximumIndexRange) << 32) | (allocation.ElementIndex & MaximumIndexRange);
				return result;
			}

			static PoolAllocation<T> UnpackIndexToAllocation(const uint64_t index)
			{
				constexpr uint64_t Mask = 0x00000000ffffffff;
				return PoolAllocation<T>{ .ChunkIndex = (index >> 32) & Mask, .ElementIndex = index & Mask };
			}

			template <typename... Args>
			uint64_t Create(const String name, Args&&... args)
			{
				PoolAllocation<T> allocation;
				{
					WriteLock lock{ poolMutex };
					allocation = instancePool.Allocate(std::forward<Args>(args)...);
				}

				const uint64_t packedIndex = PackAllocationToIndex(allocation);
				{
					WriteLock lock{ nameMapMutex };
					nameMap[packedIndex] = name;
				}

				return packedIndex;
			}

			void Destroy(const uint64_t index)
			{
				check(index != InvalidIndex);
				if (IsValid(index))
				{
					{
						WriteLock lock{ poolMutex };
						Deallocation(index);
					}

					{
						WriteLock lock{ nameMapMutex };
						nameMap.erase(index);
					}
				}
			}

			void Deallocation(const uint64_t index)
			{
				const PoolAllocation<T> allocation = UnpackIndexToAllocation(index);
				instancePool.Deallocate(allocation);
			}

			bool IsValidUnsafe(const uint64_t index) const
			{
				if (index == InvalidIndex)
				{
					return false;
				}

				return nameMap.contains(index);
			}

			bool IsValid(const uint64_t index) const
			{
				ReadOnlyLock lock{ nameMapMutex };
				return IsValidUnsafe(index);
			}

			String QueryName(const uint64_t index) const
			{
				if (index == InvalidIndex)
				{
					return {};
				}

				ReadOnlyLock lock{ nameMapMutex };
				if (!nameMap.contains(index))
				{
					return {};
				}

				return nameMap.find(index)->second;
			}

			void Rename(const uint64_t index, const String newName)
			{
				WriteLock lock{ nameMapMutex };
				if (IsValidUnsafe(index))
				{
					nameMap[index] = newName;
				}
			}

			T* QueryAddressOfInstance(const uint64_t index) const
			{
				if (IsValid(index))
				{
					return instancePool.GetAddressOfAllocation(UnpackIndexToAllocation(index));
				}

				return nullptr;
			}

		private:
			mutable SharedMutex poolMutex;
			Pool<T>				instancePool;

			mutable SharedMutex							nameMapMutex;
			robin_hood::unordered_map<uint64_t, String> nameMap;
		};
	} // namespace Private

	class HandleManager final
	{
		template <typename T>
		friend class WeakHandle;

		template <typename T>
		friend class UniqueHandle;

	public:
		HandleManager() = default;
		~HandleManager()
		{
			for (auto& repositoryPair : repositoryMap)
			{
				delete repositoryPair.second;
			}
		}

		HandleManager(const HandleManager&) = delete;
		HandleManager(HandleManager&&) noexcept = delete;

		HandleManager& operator=(const HandleManager&) = delete;
		HandleManager& operator=(HandleManager&&) = delete;

	private:
		template <typename T, typename... Args>
		uint64_t Create(const String name, Args&&... args)
		{
			constexpr uint64_t hashOfType = HashOfType<T>;
			{
				WriteLock lock{ repositoryMapMutex };
				if (!repositoryMap.contains(hashOfType))
				{
					repositoryMap[hashOfType] = static_cast<Private::HandleRepository*>(new Private::GenericHandleRepository<T>());
				}
			}

			Private::GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository->Create(name, std::forward<Args>(args)...);
		}

		template <typename T>
		void Destroy(const uint64_t handle)
		{
			Private::GenericHandleRepository<T>* repository = GetRepository<T>();
			if (repository != nullptr)
			{
				repository->Destroy(handle);
			}
		}

		template <typename T>
		Private::GenericHandleRepository<T>* GetRepository() const
		{
			ReadOnlyLock lock{ repositoryMapMutex };
			const auto	 itr = repositoryMap.find(HashOfType<T>);
			return itr != repositoryMap.end() ? static_cast<Private::GenericHandleRepository<T>*>(itr->second)
											  : nullptr;
		}

		template <typename T>
		T* QueryAddressOfInstance(const uint64_t index) const
		{
			const Private::GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? nullptr : repository->QueryAddressOfInstance(index);
		}

		template <typename T>
		String QueryName(const uint64_t index) const
		{
			const Private::GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? String() : repository->QueryName(index);
		}

		template <typename T>
		bool IsValidIndex(const uint64_t index) const
		{
			if (index == InvalidIndex)
			{
				return false;
			}

			const Private::GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? false : repository->IsValid(index);
		}

		template <typename T>
		void Rename(const uint64_t index, const String newName)
		{
			if (Private::GenericHandleRepository<T>* repository = GetRepository<T>(); repository != nullptr)
			{
				repository->Rename(index, newName);
			}
		}

	private:
		mutable SharedMutex												repositoryMapMutex;
		robin_hood::unordered_map<uint64_t, Private::HandleRepository*> repositoryMap;
	};

	template <typename T>
	class WeakHandle
	{
	public:
		using AlterConstnessType = std::conditional_t<std::is_const_v<T>, std::remove_const_t<T>, const T>;
		friend WeakHandle<AlterConstnessType>;

		WeakHandle() = default;
		WeakHandle(const WeakHandle&) = default;
		WeakHandle(WeakHandle&&) noexcept = default;
		~WeakHandle() = default;

		WeakHandle& operator=(const WeakHandle&) = default;
		WeakHandle& operator=(WeakHandle&&) noexcept = default;

		bool IsValid() const
		{
			return handleManager != nullptr && index != InvalidIndex && handleManager->IsValidIndex<T>(index);
		}

		operator bool() const { return IsValid(); }

		bool operator==(const WeakHandle& rhs) const { return index != InvalidIndex && this->index == rhs.index; }

		bool operator!=(const WeakHandle& rhs) const { return index != InvalidIndex && !(*this == rhs); }

		T& operator*()
		{
			check(IsValid());
			T* const addressOfInstance = QueryAddressOfInstance();
			check(addressOfInstance != nullptr);
			return *addressOfInstance;
		}

		const T& operator*() const
		{
			check(IsValid());
			T* const addressOfInstance = QueryAddressOfInstance();
			verify(addressOfInstance != nullptr);
			return *addressOfInstance;
		}

		T* operator->()
		{
			check(IsValid());
			T* const addressOfInstance = QueryAddressOfInstance();
			check(addressOfInstance != nullptr);
			return addressOfInstance;
		}

		const T* operator->() const
		{
			check(IsValid());
			const T* const addressOfInstance = QueryAddressOfInstance();
			check(addressOfInstance != nullptr);
			return addressOfInstance;
		}

		String QueryName() const
		{
			check(IsValid() && "Trying to query name of invalid handle.");
			if (IsValid())
			{
				return handleManager->QueryName<T>(index);
			}

			return {};
		}

		void Rename(const String newName)
		{
			check(IsValid() && "Trying to rename invalid handle.");
			if (IsValid())
			{
				handleManager->Rename<T>(index, newName);
			}
		}

		WeakHandle<T> DeriveWeak()
		{
			check(IsValid() && "Trying to derive new weak handle from invalid handle.");
			if (this->IsValid())
			{
				return WeakHandle<T>{ *(this->handleManager), this->index };
			}

			return {};
		}

		WeakHandle<const T> DeriveWeak() const
		{
			check(IsValid() && "Trying to derive new weak handle from invalid handle.");
			if (this->IsValid())
			{
				return WeakHandle<const T>{ *(this->handleManager), this->index };
			}

			return WeakHandle<const T>{};
		}

	protected:
		WeakHandle(HandleManager& owner, const uint64_t index)
			: handleManager(&owner), index(index)
		{
		}

	private:
		T* QueryAddressOfInstance() const
		{
			if (handleManager == nullptr || index == InvalidIndex)
			{
				return nullptr;
			}

			return handleManager->QueryAddressOfInstance<T>(index);
		}

	protected:
		HandleManager* handleManager = nullptr;
		uint64_t	   index = InvalidIndex;
	};

	template <typename T>
	class UniqueHandle : public WeakHandle<T>
	{
	public:
		UniqueHandle() = default;
		UniqueHandle(UniqueHandle&& other) noexcept
			: WeakHandle<T>(std::exchange(other.handleManager, nullptr), std::exchange(other.index, InvalidIndex))
		{
		}
		~UniqueHandle() { Destroy(); }

		UniqueHandle& operator=(const UniqueHandle&) = delete;
		UniqueHandle& operator=(UniqueHandle&& other) noexcept
		{
			this->handleManager = std::exchange(other.handleManager, nullptr);
			this->index = std::exchange(other.index, InvalidIndex);
			return *this;
		}

		void Destroy()
		{
			if (this->IsValid())
			{
				this->handleManager->Destroy<T>(this->index);
				this->handleManager = nullptr;
				this->index = InvalidIndex;
			}
		}

	public:
		template <typename... Args>
		static UniqueHandle Create(HandleManager& handleManager, const String name, Args&&... args)
		{
			return UniqueHandle(handleManager, handleManager.Create<T>(name, std::forward<Args>(args)...));
		}

		template <typename... Args>
		static UniqueHandle Create(HandleManager& handleManager, const std::string_view name, Args&&... args)
		{
			return Create(handleManager, String(name), std::forward<Args>(args)...);
		}

	private:
		UniqueHandle(HandleManager& owner, const uint64_t index)
			: WeakHandle<T>(owner, index) {}
	};
} // namespace fe
