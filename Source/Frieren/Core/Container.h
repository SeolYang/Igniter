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

/* #sy_deprecate_it */
#define UNUSED(x) (x)

namespace fe
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

    /* #sy_deprecate_it */
    template <typename T>
    //[[deprecated("Replace it to ranges/views")]]
    auto TransformToNative(const std::span<T> items)
    {
        std::vector<typename T::NativeType*> natives;
        natives.resize(items.size());
        std::transform(items.begin(), items.end(), natives.begin(), [](T& native)
                       { return &native.GetNative(); });
        return natives;
    }

    template <typename T>
    //[[deprecated("Replace it to ranges/views")]]
    auto TransformToNative(const std::span<const T> items)
    {
        std::vector<const typename T::NativeType*> natives;
        natives.resize(items.size());
        std::transform(items.cbegin(), items.cend(), natives.begin(),
                       [](const T& native)
                       { return &native.GetNative(); });
        return natives;
    }

    template <typename T>
    //[[deprecated("Replace it to ranges/views")]]
    auto TransformToNative(const std::span<std::reference_wrapper<T>> items)
    {
        std::vector<typename T::NativeType*> natives;
        natives.resize(items.size());
        std::transform(items.begin(), items.end(), natives.begin(),
                       [](std::reference_wrapper<T>& nativeRef)
                       { return &nativeRef.get().GetNative(); });
        return natives;
    }

    template <typename T>
    //[[deprecated("Replace it to ranges/views")]]
    auto TransformToNative(const std::span<std::reference_wrapper<const T>> items)
    {
        std::vector<const typename T::NativeType*> natives;
        natives.resize(items.size());
        std::transform(items.cbegin(), items.cend(), natives.begin(),
                       [](std::reference_wrapper<const T>& nativeRef)
                       { return &nativeRef.get().GetNative(); });
        return natives;
    }
} // namespace fe