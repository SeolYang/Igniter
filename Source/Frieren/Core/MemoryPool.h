#pragma once
#include <Core/Container.h>

namespace fe
{
	class MemoryChunk;

	/*
	 * POD 64 bits unsigned integer handle based dynamic memory pool.
	 * Does not guarantee thread-safety.
	 * Does not guarantee the invocation of a constructor or destructor. It must be called manually only if necessary.
	 */
	class MemoryPool final
	{
	public:
		MemoryPool();
		MemoryPool(const size_t newSizeOfElement, const size_t newAlignOfElement, const uint16_t numElementPerChunk, const uint32_t numInitialChunk);
		MemoryPool(const MemoryPool&) = delete;
		MemoryPool(MemoryPool&& other) noexcept;
		~MemoryPool();

		MemoryPool& operator=(const MemoryPool&) = delete;
		MemoryPool& operator=(MemoryPool&&) noexcept = delete;

		uint64_t Allocate();
		void	 Deallocate(const uint64_t handle);

		uint8_t*	   GetAddressOf(const uint64_t handle);
		const uint8_t* GetAddressOf(const uint64_t handle) const;

		bool IsAlive(const uint64_t handle) const { return handle != InvalidHandle && used.contains(handle); }

	private:
		bool IsFull() const;

		bool CreateNewChunk();

		static uint16_t GenerateMagicNumber();
		static uint16_t MaskMagicNumber(const uint64_t handle);
		static uint32_t MaskChunkIndex(const uint64_t handle);
		static uint16_t MaskElementIndex(const uint64_t handle);
		static uint64_t MakeHandle(const uint16_t magicNumber, const uint32_t chunkIdx, const uint32_t elementIdx);

	private:
		size_t								sizeOfElement;
		size_t								alignOfElement;
		uint16_t							numInitialElementPerChunk;
		uint16_t							magicNumber;
		std::vector<MemoryChunk>			chunks;
		std::queue<uint64_t>				pool;
		robin_hood::unordered_set<uint64_t> used;

		static constexpr size_t	  MagicNumberOffset = 48;
		static constexpr size_t	  ChunkIndexOffset = 16;
		static constexpr size_t	  ElementIndexOffset = 0;
		static constexpr uint32_t MaxNumChunk = 0xffffffff;
		static constexpr uint16_t MaxNumElement = 0xffff;
		static constexpr uint16_t InvalidMagicNumber = 0xffff;
		static constexpr uint64_t InvalidHandle = 0xffffffffffffffffUi64;
	};

	template <typename T>
	MemoryPool MakeMemoryPool(const uint16_t numElementPerChunk, const uint32_t numInitialChunk)
	{
		return { sizeof(T), alignof(T), numElementPerChunk, numInitialChunk };
	}
} // namespace fe