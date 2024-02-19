#include <Core/MemoryPool.h>
#include <Core/MathUtils.h>
#include <Core/MemUtils.h>
#include <Core/Assert.h>
#include <random>

namespace fe
{
	class MemoryChunk final
	{
	public:
		MemoryChunk(const size_t sizeOfChunk, const size_t alignment)
			: size(sizeOfChunk), chunk(reinterpret_cast<uint8_t*>(_aligned_malloc(sizeOfChunk, alignment)))
		{
			check(size > 0);
			check(alignment > 0 && IsPowOf2(alignment));
			check((size % alignment) == 0);
			check(chunk != nullptr);
		}

		MemoryChunk(MemoryChunk&& other) noexcept
			: chunk(std::exchange(other.chunk, nullptr)), size(other.size)
		{
		}

		~MemoryChunk()
		{
			if (chunk != nullptr)
			{
				_aligned_free(reinterpret_cast<void*>(chunk));
			}
		}

		MemoryChunk(const MemoryChunk&) = delete;
		MemoryChunk& operator=(const MemoryChunk&) = delete;
		MemoryChunk& operator=(MemoryChunk&&) noexcept = delete;

		size_t GetSize() const
		{
			return size;
		}

		uint8_t* GetAddress(const size_t offset = 0)
		{
			check(chunk != nullptr);
			check(offset < size);
			return chunk + offset;
		}

		const uint8_t* GetAddress(const size_t offset = 0) const
		{
			check(chunk != nullptr);
			check(offset < size);
			return chunk + offset;
		}

	private:
		uint8_t*	 chunk = nullptr;
		const size_t size;
	};

	MemoryPool::MemoryPool()
		: sizeOfElement(0), alignOfElement(0), numInitialElementPerChunk(0), magicNumber(InvalidMagicNumber)
	{
	}

	MemoryPool::MemoryPool(const size_t newSizeOfElement, const size_t newAlignOfElement, const uint16_t numElementPerChunk, const uint32_t numInitialChunk)
		: sizeOfElement(newSizeOfElement), alignOfElement(newAlignOfElement), numInitialElementPerChunk(numElementPerChunk), magicNumber(GenerateMagicNumber())
	{
		check(numElementPerChunk > 0);
		check(numInitialChunk > 0);

		chunks.reserve(numInitialChunk);
		pools.reserve(numInitialChunk);

		for (uint32_t chunkIdx = 0; chunkIdx < numInitialChunk; ++chunkIdx)
		{
			CreateNewChunk();
		}
	}

	MemoryPool::MemoryPool(MemoryPool&& other) noexcept
		: sizeOfElement(std::exchange(other.sizeOfElement, 0)), alignOfElement(std::exchange(other.alignOfElement, 0)), numInitialElementPerChunk(std::exchange(other.numInitialElementPerChunk, 0Ui16)), magicNumber(std::exchange(other.magicNumber, InvalidMagicNumber)), chunks(std::move(other.chunks)), pools(std::move(other.pools))
	{
	}

	MemoryPool::~MemoryPool()
	{
		check((magicNumber == InvalidMagicNumber) || IsFull());
		pools.clear();
		chunks.clear();
	}

	uint64_t MemoryPool::Allocate()
	{
		check(chunks.size() < MaxNumChunk);
		check(chunks.size() == pools.size());
		for (uint32_t chunkIdx = 0; chunkIdx < chunks.size(); ++chunkIdx)
		{
			auto& pool = pools[chunkIdx];
			if (!pool.empty())
			{
				return CreateNewHandle(chunkIdx);
			}
		}

		return CreateNewHandle(CreateNewChunk());
	}

	void MemoryPool::Deallocate(const uint64_t handle)
	{
		if (IsAlive(handle))
		{
			const uint32_t chunkIdx = MaskChunkIndex(handle);
			const uint16_t elementIdx = MaskElementIndex(handle);
			check(elementIdx < numInitialElementPerChunk);
			check(chunkIdx < chunks.size() && chunks.size() == pools.size());
			check(!pools[chunkIdx].contains(elementIdx));
			pools[chunkIdx].insert(elementIdx);
		}
		else
		{
			checkNoEntry();
		}
	}

	uint8_t* MemoryPool::GetAddressOf(const uint64_t handle)
	{
		if (IsAlive(handle))
		{
			const uint32_t chunkIdx = MaskChunkIndex(handle);
			const uint16_t elementIdx = MaskElementIndex(handle);

			check(chunkIdx < chunks.size());
			check(elementIdx < numInitialElementPerChunk);
			return chunks[chunkIdx].GetAddress(elementIdx * sizeOfElement);
		}

		checkNoEntry();
		return nullptr;
	}

	const uint8_t* MemoryPool::GetAddressOf(const uint64_t handle) const
	{
		if (IsAlive(handle))
		{
			const uint32_t chunkIdx = MaskChunkIndex(handle);
			const uint16_t elementIdx = MaskElementIndex(handle);

			check(chunkIdx < chunks.size());
			check(elementIdx < numInitialElementPerChunk);
			return chunks[chunkIdx].GetAddress(elementIdx * sizeOfElement);
		}

		checkNoEntry();
		return nullptr;
	}

	bool MemoryPool::IsAlive(const uint64_t handle) const
	{
		if (MaskMagicNumber(handle) == magicNumber)
		{
			if (const uint32_t chunkIdx = MaskChunkIndex(handle);
				chunkIdx < chunks.size())
			{
				check(chunks.size() == pools.size());
				const uint16_t elementIdx = MaskElementIndex(handle);
				return elementIdx < numInitialElementPerChunk && !pools[chunkIdx].contains(elementIdx);
			}
		}

		return false;
	}

	uint64_t MemoryPool::CreateNewHandle(const uint32_t chunkIdx)
	{
		check(chunkIdx < MaxNumChunk && chunkIdx < chunks.size());
		check(chunks.size() == pools.size());
		auto& pool = pools[chunkIdx];

		const auto	   elementIdxItr = pool.begin();
		const uint16_t elementIdx = static_cast<uint16_t>(*elementIdxItr);
		check(elementIdx < MaxNumElement);
		pool.erase(elementIdxItr);

		const uint64_t newHandle = MakeHandle(magicNumber, chunkIdx, elementIdx);
		check(newHandle != InvalidHandle);
		return newHandle;
	}

	uint32_t MemoryPool::CreateNewChunk()
	{
		if (chunks.size() + 1 == MaxNumChunk)
		{
			return MaxNumChunk;
		}

		const uint32_t newChunkIdx = static_cast<uint32_t>(chunks.size());
		chunks.emplace_back(sizeOfElement * numInitialElementPerChunk, alignOfElement);

		robin_hood::unordered_set<uint16_t> newPool;
		for (uint16_t elementIdx = 0; elementIdx < numInitialElementPerChunk; ++elementIdx)
		{
			newPool.insert(elementIdx);
		}

		pools.emplace_back(std::move(newPool));

		check(newChunkIdx + 1 == chunks.size());
		check(chunks.size() == pools.size());
		return newChunkIdx;
	}

	bool MemoryPool::IsFull() const
	{
		for (auto& pool : pools)
		{
			if (pool.size() != numInitialElementPerChunk)
			{
				return false;
			}
		}

		return true;
	}

	uint16_t MemoryPool::GenerateMagicNumber()
	{
		static std::mt19937_64 generator{ std::random_device{}() };
		std::uniform_int_distribution<uint16_t> dist{ };
		return dist(generator);
	}

	uint16_t MemoryPool::MaskMagicNumber(const uint64_t handle)
	{
		return static_cast<uint16_t>(handle >> MagicNumberOffset);
	}

	uint32_t MemoryPool::MaskChunkIndex(const uint64_t handle)
	{
		return static_cast<uint32_t>((handle >> ChunkIndexOffset) & 0xffffffffUi64);
	}

	uint16_t MemoryPool::MaskElementIndex(const uint64_t handle)
	{
		return static_cast<uint16_t>(handle & 0xffffUi64);
	}

	uint64_t MemoryPool::MakeHandle(const uint16_t magicNumber, const uint32_t chunkIdx, const uint32_t elementIdx)
	{
		return static_cast<uint64_t>(magicNumber) << MagicNumberOffset |
			   static_cast<uint64_t>(chunkIdx) << ChunkIndexOffset |
			   static_cast<uint64_t>(elementIdx);
	}
} // namespace fe