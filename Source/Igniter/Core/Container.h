#pragma once
#include <array>
#include <bitset>
#include <functional>
#include <optional>
#include <queue>
#include <ranges>
#include <span>
#include <variant>
#include <vector>
#include <chrono>
#include <concurrent_queue.h>

#pragma warning(push)
#pragma warning(disable : 26800)
#pragma warning(disable : 26819)
#pragma warning(disable : 6305)
#include <robin_hood.h>
#pragma warning(pop)
#include <Core/Json.h>
#include <crossguid/guid.hpp>
#include <magic_enum.hpp>

namespace ig
{
    using namespace std::chrono_literals;
    namespace chrono = std::chrono;
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