#pragma once
#include <Core/String.h>
#include <Core/Name.h>
#include <Core/Pool.h>
#include <Core/Mutex.h>
#include <Core/Misc.h>

namespace fe
{
	constexpr uint64_t InvalidIndex = 0xffffffffffffffffUi64;

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
		friend class HandleManager;

	public:
		GenericHandleRepository()
			: HandleRepository()
		{
		}

		virtual ~GenericHandleRepository()
		{
			const bool bNoRemainHandles = nameMap.empty();
			FE_ASSERT(bNoRemainHandles, "Some handles does not destroyed.");
			if (!bNoRemainHandles)
			{
				for (const auto namePair : nameMap)
				{
					Deallocation(namePair.first);
				}
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
			FE_ASSERT(allocation.ChunkIndex < MaximumIndexRange || allocation.ElementIndex < MaximumIndexRange, "Outranged Allocation to pack.");
			uint64_t result = 0;
			result = ((allocation.ChunkIndex & MaximumIndexRange) << 32) & (allocation.ElementIndex & MaximumIndexRange);
			return result;
		}

		static PoolAllocation<T> UnpackIndexToAllocation(const uint64_t index)
		{
			constexpr uint64_t Mask = 0x00000000ffffffff;
			return PoolAllocation<T>{
				.ChunkIndex = (index >> 32) & Mask,
				.ElementIndex = index & Mask
			};
		}

		template <typename... Args>
		uint64_t Create(const Name name, Args&&... args)
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
			FE_ASSERT(index != InvalidIndex, "Trying to delete invalid index.");
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

		bool IsValid(const uint64_t index) const
		{
			if (index == InvalidIndex)
			{
				return false;
			}

			ReadOnlyLock lock{ nameMapMutex };
			return nameMap.contains(index);
		}

		Name QueryName(const uint64_t index) const
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

		mutable SharedMutex						  nameMapMutex;
		robin_hood::unordered_map<uint64_t, Name> nameMap;
	};

	class HandleManager final
	{
	public:
		HandleManager() = default;
		~HandleManager() = default;

		HandleManager(const HandleManager&) = delete;
		HandleManager(HandleManager&&) noexcept = delete;

		HandleManager& operator=(const HandleManager&) = delete;
		HandleManager& operator=(HandleManager&&) = delete;

		template <typename T>
		class WeakHandle
		{
		public:
			WeakHandle(const WeakHandle&) = default;
			WeakHandle(WeakHandle&&) noexcept = default;
			virtual ~WeakHandle()
			{
				rHandleManager = nullptr;
				index = InvalidIndex;
			}

			WeakHandle& operator=(const WeakHandle&) = default;
			WeakHandle& operator=(WeakHandle&&) noexcept = default;

			operator bool() const
			{
				return IsValid();
			}

			bool operator==(const WeakHandle& rhs) const
			{
				return this->index == rhs.index;
			}

			T& operator*()
			{
				T* const addressOfInstance = QueryAddressOfInstance();
				FE_ASSERT(addressOfInstance != nullptr, "Trying to access to invalid handle.");
				return *addressOfInstance;
			}

			const T& operator*() const
			{
				T* const addressOfInstance = QueryAddressOfInstance();
				FE_ASSERT(addressOfInstance != nullptr, "Trying to access to invalid handle.");
				return *addressOfInstance;
			}

			T* operator->()
			{
				T* const addressOfInstance = QueryAddressOfInstance();
				FE_ASSERT(addressOfInstance != nullptr, "Trying to access to invalid handle.");
				return addressOfInstance;
			}

			const T* operator->() const
			{
				const T* const addressOfInstance = QueryAddressOfInstance();
				FE_ASSERT(addressOfInstance != nullptr, "Trying to access to invalid handle.");
				return addressOfInstance;
			}

			bool IsValid() const
			{
				return rHandleManager != nullptr && index != InvalidIndex && rHandleManager->IsValidIndex<T>(index);
			}

			Name QueryName() const
			{
				if (rHandleManager == nullptr || index == InvalidIndex)
				{
					return {};
				}

				return rHandleManager->QueryName<T>(index);
			}

		protected:
			WeakHandle() = default;
			WeakHandle(HandleManager& handleManager, const uint64_t index)
				: rHandleManager(&handleManager), index(index)
			{
			}

			static WeakHandle Create(HandleManager& handleManager, const uint64_t index)
			{
				return WeakHandle{ handleManager, index };
			}

			static WeakHandle CreateInvalid()
			{
				return {};
			}

		private:
			T* QueryAddressOfInstance() const
			{
				if (rHandleManager == nullptr || index == InvalidIndex)
				{
					return nullptr;
				}

				return rHandleManager->QueryAddressOfInstance<T>(index);
			}

		protected:
			HandleManager* rHandleManager = nullptr;
			uint64_t	   index = InvalidIndex;
		};

		template <typename T>
		class OwnedHandle : public WeakHandle<T>
		{
		public:
			OwnedHandle() = default;
			OwnedHandle(OwnedHandle&& other) noexcept
				: WeakHandle<T>(std::exchange(other.rHandleManager, nullptr), std::exchange(other.index, InvalidIndex))
			{
			}
			virtual ~OwnedHandle() override
			{
				Destroy();
			}

			OwnedHandle& operator=(const OwnedHandle&) = delete;
			OwnedHandle& operator=(OwnedHandle&& other) noexcept
			{
				this->rHandleManager = std::exchange(other.rHandleManager, nullptr);
				this->index = std::exchange(other.index, InvalidIndex);
				return *this;
			}

			void Destroy()
			{
				if (this->IsValid())
				{
					this->rHandleManager->Destroy<T>(this->index);
					this->rHandleManager = nullptr;
					this->index = InvalidIndex;
				}
			}

			WeakHandle<T> DeriveWeak()
			{
				if (this->IsValid())
				{
					return WeakHandle<T>::Create(*(this->rHandleManager), this->index);
				}

				return WeakHandle<T>::CreateInvalid();
			}

		public:
			template <typename... Args>
			static OwnedHandle Create(HandleManager& handleManager, const Name name, Args&&... args)
			{
				return OwnedHandle(handleManager,
					handleManager.Create<T>(name, std::forward<Args>(args)...));
			}

			template <typename... Args>
			static OwnedHandle Create(HandleManager& handleManager, const std::string_view name, Args&&... args)
			{
				return OwnedHandle(handleManager,
					handleManager.Create<T>(Name(name), std::forward<Args>(args)...));
			}

		private:
			OwnedHandle(HandleManager& handleManager, const uint64_t index)
				: WeakHandle<T>(handleManager, index)
			{
			}
		};

	private:
		template <typename T, typename... Args>
		uint64_t Create(const Name name, Args&&... args)
		{
			constexpr uint64_t hashOfType = HashOfType<T>;
			{
				WriteLock lock{ repositoryMapMutex };
				if (!repositoryMap.contains(hashOfType))
				{
					repositoryMap[hashOfType] = static_cast<HandleRepository*>(new GenericHandleRepository<T>());
				}
			}

			GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository->Create(name, std::forward<Args>(args)...);
		}

		template <typename T>
		void Destroy(const uint64_t handle)
		{
			GenericHandleRepository<T>* repository = GetRepository<T>();
			if (repository != nullptr)
			{
				repository->Destroy(handle);
			}
		}

		template <typename T>
		GenericHandleRepository<T>* GetRepository() const
		{
			ReadOnlyLock lock{ repositoryMapMutex };
			return static_cast<GenericHandleRepository<T>*>(repositoryMap.find(HashOfType<T>)->second);
		}

		template <typename T>
		T* QueryAddressOfInstance(const uint64_t index) const
		{
			const GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? nullptr : repository->QueryAddressOfInstance(index);
		}

		template <typename T>
		Name QueryName(const uint64_t index) const
		{
			const GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? Name() : repository->QueryName(index);
		}

		template <typename T>
		bool IsValidIndex(const uint64_t index) const
		{
			if (index == InvalidIndex)
			{
				return false;
			}

			const GenericHandleRepository<T>* repository = GetRepository<T>();
			return repository == nullptr ? false : repository->IsValid(index);
		}

	private:
		mutable SharedMutex									   repositoryMapMutex;
		robin_hood::unordered_map<uint64_t, HandleRepository*> repositoryMap;
	};

	template <typename T>
	using WeakHandle = HandleManager::WeakHandle<T>;

	template <typename T>
	using OwnedHandle = HandleManager::OwnedHandle<T>;
} // namespace fe