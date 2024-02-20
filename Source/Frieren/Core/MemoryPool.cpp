#include <Core/MemoryPool.h>
#include <Core/MathUtils.h>
#include <Core/MemUtils.h>
#include <Core/Assert.h>
#include <random>

namespace fe
{

	MemoryPool::MemoryPool()
		: sizeOfElement(0), alignOfElement(0), numInitialElementPerChunk(0), magicNumber(InvalidMagicNumber)
	{
	}

	MemoryPool::MemoryPool(const size_t newSizeOfElement, const size_t newAlignOfElement, const uint16_t numElementPerChunk, const uint32_t numInitialChunk)
		: sizeOfElement(newSizeOfElement), alignOfElement(newAlignOfElement), numInitialElementPerChunk(numElementPerChunk), magicNumber(GenerateMagicNumber())
	{
		check(sizeOfElement >= sizeof(uint64_t) && "The size of element must be at least sizeof(uint64_t) due to 'a element aliveness ensuring mechanism'.");
		check(numElementPerChunk > 0);
		check(numInitialChunk > 0);
		chunks.reserve(numInitialChunk);

		for (uint32_t chunkIdx = 0; chunkIdx < numInitialChunk; ++chunkIdx)
		{
			CreateNewChunk();
		}
	}

	MemoryPool::MemoryPool(MemoryPool&& other) noexcept
		: sizeOfElement(std::exchange(other.sizeOfElement, 0)), alignOfElement(std::exchange(other.alignOfElement, 0)), numInitialElementPerChunk(std::exchange(other.numInitialElementPerChunk, 0Ui16)), magicNumber(std::exchange(other.magicNumber, InvalidMagicNumber)), chunks(std::move(other.chunks)), pool(std::move(other.pool))
	{
	}

	MemoryPool::~MemoryPool()
	{
		check((magicNumber == InvalidMagicNumber) || IsFull());
	}

	uint64_t MemoryPool::Allocate()
	{
		if (pool.empty() && !CreateNewChunk())
		{
			checkNoEntry();
			return InvalidHandle;
		}

		const uint64_t newHandle = pool.front();
		pool.pop();

		uint64_t* handleInUseFlagPtr = GetHandleInUseFlagPtr(newHandle);
		if (handleInUseFlagPtr != nullptr && (*handleInUseFlagPtr == HandleCurrentlyNotInUseFlag))
		{
			*handleInUseFlagPtr = 0;
		}
		else
		{
			checkNoEntry();
			return InvalidHandle;
		}

		return newHandle;
	}

	void MemoryPool::Deallocate(const uint64_t handle)
	{
		if (IsAlive(handle))
		{
			pool.push(handle);
		}
		else
		{
			checkNoEntry();
		}
	}

	bool MemoryPool::IsFull() const
	{
		return pool.size() == (chunks.size() * numInitialElementPerChunk);
	}

	bool MemoryPool::CreateNewChunk()
	{
		if (chunks.size() + 1 != MaxNumChunk)
		{
			const uint32_t newChunkIdx = static_cast<uint32_t>(chunks.size());
			chunks.emplace_back(sizeOfElement * numInitialElementPerChunk, alignOfElement);
			for (uint16_t elementIdx = 0; elementIdx < numInitialElementPerChunk; ++elementIdx)
			{
				const uint64_t newHandle = MakeHandle(magicNumber, newChunkIdx, elementIdx);
				check(newHandle != InvalidHandle);
				pool.push(newHandle);

				uint64_t* handleInUseFlagPtr = GetHandleInUseFlagPtr(newHandle);
				*handleInUseFlagPtr = HandleCurrentlyNotInUseFlag;
			}

			return true;
		}

		return false;
	}

	uint16_t MemoryPool::GenerateMagicNumber()
	{
		static std::mt19937_64					generator{ std::random_device{}() };
		std::uniform_int_distribution<uint16_t> dist{};
		return dist(generator);
	}
} // namespace fe