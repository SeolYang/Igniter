#pragma once
#include <memory>
#include <queue>
#include <algorithm>
#include <robin_hood.h>
#include <Core/Assert.h>

namespace fe
{
	/**
	 * Chunk and Pool either does not guarantees 'Thread-safety'.
	 */
	class Chunk
	{
	public:
		template <typename T>
		friend class Pool;

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
			FE_ASSERT(allocatedSlot.empty(), "Some slots does not returned. It may occur cause of memory leak.");
			_aligned_free(base);
		}

		bool IsValidSlot(const size_t slot) const
		{
			return allocatedSlot.contains(slot);
		}

	private:
		template <typename T>
		static std::unique_ptr<Chunk> Create(const size_t numOfElements)
		{
			FE_ASSERT(numOfElements > 0, "Zero elements chunk does not allowed.");
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
			FE_ASSERT(bIsValidSlot, "Trying to deallocate invalid slot {}.", slot);
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

	template <typename T>
	class Pool
	{
	public:
		struct Allocation
		{
			uint64_t ChunkIndex = std::numeric_limits<uint32_t>::max();
			uint64_t ElementIndex = std::numeric_limits<uint32_t>::max();
			T*		 AddressOfInstance = nullptr;
		};

	public:
		explicit Pool(const size_t numOfInitialChunks, const size_t numOfElementsPerChunk)
			: numOfElementsPerChunk(numOfElementsPerChunk)
		{
			chunks.reserve(numOfInitialChunks);
			for (size_t chunkIndex = 0; chunkIndex < numOfInitialChunks; ++chunkIndex)
			{
				chunks.emplace_back(Chunk::Create<T>(numOfElementsPerChunk));
			}
		}

		explicit Pool(const size_t numOfInitialChunks)
			: Pool(numOfInitialChunks, DefaultChunkSize / sizeof(T))
		{
		}

		~Pool() = default;

		template <typename... Args>
		Allocation Allocate(Args&&... args)
		{
			Allocation result = Allocate();
			new (result.AddressOfInstance) T(std::forward<Args>(args)...);
			return result;
		}

		void Deallocate(const Allocation allocation)
		{
			FE_ASSERT(allocation.ChunkIndex < chunks.size());
			T* properPointer = reinterpret_cast<T*>(chunks[allocation.ChunkIndex]->GetPointerOfSlot(allocation.ElementIndex));
			if (properPointer != nullptr)
			{
				properPointer->~T();
			}
			chunks[allocation.ChunkIndex]->Deallocate(allocation.ElementIndex);
		}

	private:
		Allocation Allocate()
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
				chunks.emplace_back(Chunk::Create<T>(numOfElementsPerChunk));
			}

			result.ChunkIndex = chunkIndex;
			result.ElementIndex = chunks[chunkIndex]->Allocate();
			result.AddressOfInstance = reinterpret_cast<T*>(chunks[chunkIndex]->GetPointerOfSlot(result.ElementIndex));

			return result;
		}

	private:
		constexpr static size_t				DefaultChunkSize = 16384;
		const size_t						numOfElementsPerChunk;
		std::vector<std::unique_ptr<Chunk>> chunks;
	};
} // namespace fe
