#pragma once
#include <Igniter.h>

namespace ig
{
    using DefaultCallback = std::function<void()>;

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

    template <std::ranges::range R>
    auto ToVector(R&& range)
    {
        std::vector<std::ranges::range_value_t<R>> newVec{};
        if constexpr (std::ranges::sized_range<R>)
        {
            newVec.reserve(std::ranges::size(range));
        }

        std::ranges::copy(range, std::back_inserter(newVec));
        return newVec;
    }
} // namespace ig