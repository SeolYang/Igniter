#pragma once
#include <optional>
#include <functional>
#include <Core/Assert.h>

namespace fe
{
    template <typename T>
    using Optional = std::optional<T>;

    template <typename T>
    using Ref = std::reference_wrapper<T>;

    template <typename T>
    using CRef = std::reference_wrapper<const T>;

    template <typename T>
    using OptionalRef = Optional<Ref<T>>;

    template <typename T>
    using OptionalCRef = Optional<CRef<T>>;

    template <typename T>
    T& Unwrap(OptionalRef<T>& optionalRef)
    {
		FE_ASSERT(optionalRef.has_value(), "Trying to unwrap null optional.");
		return optionalRef->get();
    }

    template <typename T>
    const T& Unwrap(OptionalCRef<T>& optionalCRef)
    {
		FE_ASSERT(optionalCRef.has_value(), "Trying to unwrap null optional.");
		return optionalCRef->get();
    }
}
