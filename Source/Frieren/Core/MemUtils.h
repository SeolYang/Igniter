#pragma once
#include <Core/Assert.h>
#include <Core/MathUtils.h>

namespace fe
{
	constexpr size_t CalculateAlignAdjustment(const size_t size, const size_t alignment)
	{
		check(size >= sizeof(void*));
		check(IsPowOf2(alignment));
		const size_t mask = alignment - 1;
		const size_t misalignment = size & mask;
		return (alignment - misalignment);
	}
}