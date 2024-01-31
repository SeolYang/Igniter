#pragma once
#include <memory>
#include <queue>
#include <algorithm>
#include <robin_hood.h>
#include <Core/Assert.h>
#include <Core/Misc.h>

namespace fe
{
	template <typename T>
	class Pool;

	namespace Private
	{
		/**
		 * Chunk and Pool either does not guarantees 'Thread-safety'.
		 */
		class Chunk
		{
		public:
			template <typename T>
			friend class fe::Pool;

			explicit Chunk(const size_t sizeOfElement, const size_t alignmentOfElement, const size_t numOfElements)
				: sizeOfElement(sizeOfElement), base(_aligned_malloc(sizeOfElement * numOfElements, alignmentOfElement))
			{
				for (size_t index = 0; index < numOfElements; ++index)
				{
					freeSlots.push(index);
				}
			}

			~Chunk()
			{
				verify(allocatedSlot.empty());
				_aligned_free(base);
			}

			bool IsValidSlot(const size_t slot) const { return allocatedSlot.contains(slot); }

		private:
			template <typename T>
			static std::unique_ptr<Chunk> Create(const size_t numOfElements)
			{
				verify(numOfElements > 0);
				return std::make_unique<Chunk>(sizeof(T), alignof(T), numOfElements);
			}

			size_t Allocate()
			{
				const size_t allocated = freeSlots.front();
				freeSlots.pop();
				allocatedSlot.insert(allocated);
				return allocated;
			}

			void Deallocate(const size_t slot)
			{
				const bool bIsValidSlot = IsValidSlot(slot);
				verify(bIsValidSlot);
				if (bIsValidSlot)
				{
					freeSlots.push(slot);
					allocatedSlot.erase(slot);
				}
			}

			void* GetBasePointer() const { return base; }

			void* GetPointerOfSlot(const size_t slot) const
			{
				if (!IsValidSlot(slot))
				{
					return nullptr;
				}

				char* result = reinterpret_cast<char*>(base) + (slot * sizeOfElement);
				return reinterpret_cast<void*>(result);
			}

			bool HasAnyFreeSlot() const { return !freeSlots.empty(); }

		private:
			const size_t					  sizeOfElement;
			void*							  base;
			std::queue<size_t>				  freeSlots;
			robin_hood::unordered_set<size_t> allocatedSlot;
		};
	} // namespace Private

	template <typename T>
	class Pool
	{
	public:
		struct Allocation
		{
			uint64_t ChunkIndex = std::numeric_limits<uint32_t>::max();
			uint64_t ElementIndex = std::numeric_limits<uint32_t>::max();
		};

	public:
		explicit Pool(const size_t numOfInitialChunks, const size_t numOfElementsPerChunk)
			: numOfElementsPerChunk(numOfElementsPerChunk)
		{
			chunks.reserve(numOfInitialChunks);
			for (size_t chunkIndex = 0; chunkIndex < numOfInitialChunks; ++chunkIndex)
			{
				chunks.emplace_back(Private::Chunk::Create<T>(numOfElementsPerChunk));
			}
		}

		explicit Pool(const size_t numOfInitialChunks = 1) : Pool(numOfInitialChunks, DefaultChunkSize / sizeof(T)) {}

		~Pool() = default;

		template <typename... Args>
		[[nodiscard]] Allocation Allocate(Args&&... args)
		{
			const Allocation result = AllocateInternal();
			T* const		 addressOfInstance = GetAddressOfAllocation(result);
			new (addressOfInstance) T(std::forward<Args>(args)...);
			return result;
		}

		void Deallocate(const Allocation allocation)
		{
			verify(allocation.ChunkIndex < chunks.size());
			T* const addressOfInstance = GetAddressOfAllocation(allocation);
			if (addressOfInstance != nullptr)
			{
				addressOfInstance->~T();
			}
			chunks[allocation.ChunkIndex]->Deallocate(allocation.ElementIndex);
		}

		void SafeDeallocate(Allocation& allocation)
		{
			Deallocate(allocation);
			allocation.ChunkIndex = allocation.ElementIndex = InvalidIndex;
		}

		T* GetAddressOfAllocation(const Allocation allocation) const
		{
			const bool bIsValidChunkIndex = allocation.ChunkIndex < chunks.size();
			verify(bIsValidChunkIndex);
			return bIsValidChunkIndex
					   ? reinterpret_cast<T*>(chunks[allocation.ChunkIndex]->GetPointerOfSlot(allocation.ElementIndex))
					   : nullptr;
		}

	private:
		Allocation AllocateInternal()
		{
			Allocation result{};

			size_t chunkIndex = 0;
			for (; chunkIndex < chunks.size(); ++chunkIndex)
			{
				if (chunks[chunkIndex]->HasAnyFreeSlot())
				{
					break;
				}
			}

			if (chunkIndex == chunks.size())
			{
				chunks.emplace_back(Private::Chunk::Create<T>(numOfElementsPerChunk));
			}

			result.ChunkIndex = chunkIndex;
			result.ElementIndex = chunks[chunkIndex]->Allocate();

			return result;
		}

	private:
		constexpr static size_t						 DefaultChunkSize = 16384;
		const size_t								 numOfElementsPerChunk;
		std::vector<std::unique_ptr<Private::Chunk>> chunks;
	};

	template <typename T>
	using PoolAllocation = Pool<T>::Allocation;
} // namespace fe
