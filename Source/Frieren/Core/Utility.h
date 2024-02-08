#pragma once
#include <Core/Container.h>

#define UNUSED(x) (x)

namespace fe
{
	template <typename IndexType>
	std::queue<IndexType> CreateIndexQueue(const IndexType numIndices)
	{
		std::queue<IndexType> result;
		for (IndexType index = {}; index < numIndices; ++index)
		{
			result.push(index);
		}
		return result;
	}

	template <typename T>
	bool BitFlagContains(const T& bits, const T& flag)
	{
		return ((bits & flag) == flag);
	}

	template <typename T, typename... Args>
	bool BitFlagContains(const T& bits, const T& flag, const Args&... args)
	{
		return ((bits & flag) == flag) && BitFlagContains(bits, args...);
	}
} // namespace fe