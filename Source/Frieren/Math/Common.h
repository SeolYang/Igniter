#pragma once
#include <cmath>
#include <Core/Assert.h>

namespace fe
{
	template <typename T>
	__forceinline bool IsPow2(T value)
	{
		return ((size_t)value & (size_t)(value - 1)) == 0;
	}
	template <typename T>
	__forceinline T AlignUp(T value, const size_t alignment)
	{
		verify(IsPow2(value));
		const size_t mask = alignment - 1;
		return (T)((size_t)(value + mask) & ~mask);
	}

	template <typename T>
	__forceinline T AlignDown(T value, const size_t alignment)
	{
		verify(IsPow2(value));
		const size_t mask = alignment - 1;
		return (T)((size_t)value & ~mask);
	}
} // namespace fe