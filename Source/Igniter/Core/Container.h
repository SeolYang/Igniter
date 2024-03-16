#pragma once
#include <array>
#include <vector>
#include <queue>
#include <unordered_set>
#include <span>
#include <optional>
#include <bitset>
#include <functional>
#include <variant>
#include <ranges>

#pragma warning(push)
#pragma warning(disable : 26800)
#pragma warning(disable : 26819)
#pragma warning(disable : 6305)
#include <robin_hood.h>
#pragma warning(pop)
#include <crossguid/guid.hpp>
#include <magic_enum.hpp>
#include <Core/Json.h>

namespace ig
{
    namespace ranges = std::ranges;
    namespace views = std::views;

    template <typename T>
    using OptionalRef = std::optional<std::reference_wrapper<T>>;

    template <typename T>
    OptionalRef<T> MakeOptionalRef(T& ref)
    {
        return std::make_optional(std::ref(ref));
    }

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
} // namespace ig