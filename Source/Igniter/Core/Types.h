#pragma once
#include <Igniter.h>

namespace ig
{
    namespace fs = std::filesystem;
    namespace chrono = std::chrono;
    namespace ranges = std::ranges;
    namespace views = std::views;

    using namespace DirectX::SimpleMath;
    using namespace std::chrono_literals;

    using json = nlohmann::json;
    using SharedMutex = std::shared_mutex;
    using ReadOnlyLock = std::shared_lock<SharedMutex>;
    using ReadWriteLock = std::unique_lock<SharedMutex>;
    using RecursiveMutex = std::recursive_mutex;
    using RecursiveLock = std::unique_lock<RecursiveMutex>;
    using Mutex = std::mutex;
    using UniqueLock = std::unique_lock<Mutex>;

    template <typename T>
    using OptionalRef = std::optional<std::reference_wrapper<T>>;
    template <typename T>
    OptionalRef<T> MakeOptionalRef(T& ref)
    {
        return std::make_optional(std::ref(ref));
    }

    template <typename T>
    using Ptr = std::unique_ptr<T>;

    template <typename T>
    using SharedPtr = std::shared_ptr<T>;

    using Entity = entt::entity;
    using Registry = entt::registry;

    constexpr uint64_t InvalidIndex = 0xffffffffffffffffUi64;
    constexpr uint32_t InvalidIndexU32 = 0xffffffffU;

    template <typename T>
        requires std::is_arithmetic_v<std::decay_t<T>>
    consteval T NumericMinOfValue(const T&)
    {
        return std::numeric_limits<std::decay_t<T>>::min();
    }

    template <typename T>
        requires std::is_arithmetic_v<std::decay_t<T>>
    consteval T NumericMaxOfValue(const T&)
    {
        return std::numeric_limits<std::decay_t<T>>::max();
    }

    template <typename T, typename F>
    constexpr bool ContainsFlags(const T& flags, const F& flag)
    {
        const auto bits = static_cast<F>(flags);
        return ((bits & flag) == flag);
    }
} // namespace ig

#define IG_NUMERIC_MIN_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::min()
#define IG_NUMERIC_MAX_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::max()

#define IG_ENUM_FLAGS(ENUM_TYPE)                                                            \
    inline constexpr ENUM_TYPE operator|(const ENUM_TYPE lhs, const ENUM_TYPE rhs)          \
    {                                                                                       \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                           \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) | \
                                      static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }                                                                                       \
    inline constexpr ENUM_TYPE operator&(const ENUM_TYPE lhs, const ENUM_TYPE rhs)          \
    {                                                                                       \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                           \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) & \
                                      static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }