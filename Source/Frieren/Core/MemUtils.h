#pragma once
#include <Core/Assert.h>
#include <Math/Utils.h>
#include <memory>

namespace fe
{
	constexpr uint64_t InvalidIndex = 0xffffffffffffffffUi64;
	constexpr uint32_t InvalidIndexU32 = 0xffffffffU;
	constexpr size_t CalculateAlignAdjustment(const size_t size, const size_t alignment)
	{
		check(size >= sizeof(void*));
		check(IsPowOf2(alignment));
		const size_t mask = alignment - 1;
		const size_t misalignment = size & mask;
		return (alignment - misalignment);
	}

	inline uint64_t AlignTo(const uint64_t val, const uint64_t alignment)
	{
		check(alignment > 0 && IsPowOf2(alignment));
		return ((val + alignment - 1) / alignment) * alignment;
	}
}