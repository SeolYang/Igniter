#pragma once
#include "Igniter/Igniter.h"

namespace ig
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

    template <std::ranges::range R>
    auto ToVector(R&& range)
    {
        eastl::vector<std::ranges::range_value_t<R>> newVec{};
        if constexpr (std::ranges::sized_range<R>)
        {
            newVec.reserve(std::ranges::size(range));
        }

        std::ranges::copy(range, std::back_inserter(newVec));
        return newVec;
    }

    template <typename T, typename... Params>
        requires(std::is_same_v<T, Params> && ...)
    auto MakeRefArray(T& reference, [[maybe_unused]] Params&... params)
    {
        return std::array<Ref<T>, sizeof...(Params) + 1>{std::ref(reference), std::ref(params)...};
    }
}    // namespace ig
