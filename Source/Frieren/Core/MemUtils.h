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

		template <typename T>
	__forceinline T AlignUp(T value, const size_t alignment)
	{
		check(IsPow2(value));
		const size_t mask = alignment - 1;
		return (T)((size_t)(value + mask) & ~mask);
	}

	template <typename T>
	__forceinline T AlignDown(T value, const size_t alignment)
	{
		check(IsPow2(value));
		const size_t mask = alignment - 1;
		return (T)((size_t)value & ~mask);
	}
}