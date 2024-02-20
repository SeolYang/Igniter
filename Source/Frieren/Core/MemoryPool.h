#pragma once
#include <Core/Container.h>
#include <Core/Assert.h>
#include <Core/MathUtils.h>

namespace fe
{
	/*
	 * POD 64 bits unsigned integer handle based dynamic memory pool.
	 * Does not guarantee thread-safety.
	 * Does not guarantee the invocation of a constructor or destructor. It must be called manually only if necessary.
	 * #further	헤더 기반으로 4~8바이트 크기의 헤더를 추가로 사용하여, 버전 관리 및 상태관리 기능 확장
	 * ex. 8바이트(64비트) 헤더 -> 32비트 = 버전/8비트 = 상태/24비트 = 예약
	 * GetAddressOf(handle, version) -> if version != header.version -> return nullptr
	 * 추후 WeakHandle의 유효성 판정에 사용
	 * Magic Number 이용한 판정 -> 헤더 상태 확인으로 변경
	 * IsAlive(handle) -> returnh header.status == EStatus::Alive
	 * (header size + element size) = newSize, which require newSize % max(element align, header align) == 0
	 * ex. Header size == 8 bytes & alignment == 8 bytes; Element size == 4 bytes & alignment == 4 bytes
	 * newSize == 16; (Header Size + Element Size) + Adjustment<4 bytes> = 16 % 8 bytes == 0 ... 12 bytes of extra mem costs..
	 * (300% of extra mem costs)
	 * ex2. Header size/alignment same; Element Size == 96 bytes & alignment == 8 bytes
	 * newSize == 104 bytes, 104 % 8 == 0, (7.7% of extra mem costs)
	 * 데이터 자체의 크기가 커질 수록 헤더로 인해 발생하는 추가 메모리 비용은 상대적으로 작아진다.
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
			return IsValid(handle) ? (chunks[MaskChunkIndex(handle)] + GetElementOffset(MaskElementIndex(handle)))
				: nullptr;
		}

		const uint8_t* GetAddressOf(const uint64_t handle) const
		{
			return IsValid(handle) ? (chunks[MaskChunkIndex(handle)] + GetElementOffset(MaskElementIndex(handle)))
				: nullptr;
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
		size_t					 sizeOfElement;
		size_t					 alignOfElement;
		uint16_t				 numInitialElementPerChunk;
		uint16_t				 magicNumber;
		std::vector<uint8_t*>	 chunks;
		std::queue<uint64_t>	 pool;

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
	};

	template <typename T>
	MemoryPool MakeMemoryPool(const uint16_t numElementPerChunk, const uint32_t numInitialChunk)
	{
		return { sizeof(T), alignof(T), numElementPerChunk, numInitialChunk };
	}
} // namespace fe