#pragma once
#include <Core/Container.h>
#include <Core/Assert.h>
#include <Core/MathUtils.h>

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

		uint8_t* GetAddressOf(const uint64_t handle)
		{
			return IsValid(handle) ? chunks[MaskChunkIndex(handle)].GetAddress(GetElementOffset(MaskElementIndex(handle))) : nullptr;
		}

		const uint8_t* GetAddressOf(const uint64_t handle) const
		{
			return IsValid(handle) ? chunks[MaskChunkIndex(handle)].GetAddress(GetElementOffset(MaskElementIndex(handle))) : nullptr;
		}

		bool IsValid(const uint64_t handle) const
		{
			return handle != InvalidHandle && ((MaskMagicNumber(handle) == magicNumber) &&
											   (MaskChunkIndex(handle) < chunks.size()) &&
											   (MaskElementIndex(handle) < numInitialElementPerChunk));
		}

		bool IsAlive(const uint64_t handle) const
		{
			const uint64_t* flagPtr = GetHandleInUseFlagPtr(handle);
			return flagPtr != nullptr ? (*flagPtr != HandleCurrentlyNotInUseFlag) : false;
		}

	private:
		bool IsFull() const;

		bool CreateNewChunk();

		static uint16_t GenerateMagicNumber();
		static uint16_t MaskMagicNumber(const uint64_t handle) { return static_cast<uint16_t>(handle >> MagicNumberOffset); }
		static uint32_t MaskChunkIndex(const uint64_t handle) { return static_cast<uint32_t>((handle >> ChunkIndexOffset) & 0xffffffffUi64); }
		static uint16_t MaskElementIndex(const uint64_t handle) { return static_cast<uint16_t>(handle & 0xffffUi64); }
		static uint64_t MakeHandle(const uint16_t magicNumber, const uint32_t chunkIdx, const uint32_t elementIdx)
		{
			return static_cast<uint64_t>(magicNumber) << MagicNumberOffset |
				   static_cast<uint64_t>(chunkIdx) << ChunkIndexOffset |
				   static_cast<uint64_t>(elementIdx);
		}

		size_t GetElementOffset(uint16_t elementIdx) const { return sizeOfElement * elementIdx; }

		uint64_t*		GetHandleInUseFlagPtr(const uint64_t handle) { return reinterpret_cast<uint64_t*>(GetAddressOf(handle)); }
		const uint64_t* GetHandleInUseFlagPtr(const uint64_t handle) const { return reinterpret_cast<const uint64_t*>(GetAddressOf(handle)); }

	private:
		size_t								sizeOfElement;
		size_t								alignOfElement;
		uint16_t							numInitialElementPerChunk;
		uint16_t							magicNumber;
		std::vector<MemoryChunk>			chunks;
		std::queue<uint64_t>				pool;

		static constexpr size_t	  MagicNumberOffset = 48;
		static constexpr size_t	  ChunkIndexOffset = 16;
		static constexpr size_t	  ElementIndexOffset = 0;
		static constexpr uint32_t MaxNumChunk = 0xffffffff;
		static constexpr uint16_t MaxNumElement = 0xffff;
		static constexpr uint16_t InvalidMagicNumber = 0xffff;
		static constexpr uint64_t InvalidHandle = 0xffffffffffffffffUi64;

		/*
		 * Handle currently not in use.
		 * H(8) A(A) N(9) D(D) L(1) E(E) Currently(C) N(9) O(0) T(2) I(5) N(9) U(7) S(6) E(E) .(3)
		 * 8A9D1EC9025976E3
		 */
		static constexpr uint64_t HandleCurrentlyNotInUseFlag = 0x8A9D1EC9025976E3;

		/*
		 * Handle currently not in use.
		 * H(8) A(A) N(9) D(D) L(1) E(E) Currently(C) N(9) O(0) T(2) I(5) N(9) U(7) S(6) E(E) .(3)
		 * 8A9D1EC9025976E3
		 */
	};

	template <typename T>
	MemoryPool MakeMemoryPool(const uint16_t numElementPerChunk, const uint32_t numInitialChunk)
	{
		return { sizeof(T), alignof(T), numElementPerChunk, numInitialChunk };
	}
} // namespace fe