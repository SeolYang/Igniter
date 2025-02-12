#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    namespace fs = std::filesystem;
    namespace chrono = std::chrono;
    namespace ranges = std::ranges;
    namespace views = std::views;
    using namespace DirectX::SimpleMath;
    using namespace std::chrono_literals;

    using U8 = uint8_t;
    using U16 = uint16_t;
    using U32 = uint32_t;
    using U64 = uint64_t;
    using F32 = float;
    using F64 = double;
    using I8 = int8_t;
    using I16 = int16_t;
    using I32 = int32_t;
    using I64 = int64_t;
    using Size = size_t;
    using Bytes = Size;
    using KiloBytes = Size;
    using MegaBytes = Size;
    using GigaBytes = Size;
    using Index = Size;
    constexpr Index InvalidIndex = 0xffffffffffffffffUi64;
    constexpr U32 InvalidIndexU32 = 0xffffffffU;
    using GlobalFrameIndex = Index;
    using LocalFrameIndex = U8;

    template <typename T, typename Dx = std::default_delete<T>>
    using Ptr = std::unique_ptr<T, Dx>;
    template <typename T>
    using SharedPtr = std::shared_ptr<T>;
    template <typename T>
    using Ref = std::reference_wrapper<T>;
    template <typename T>
    using CRef = Ref<const T>;

    using Path = std::filesystem::path;

    template <typename Key, typename T>
    using OrderedMap = eastl::map<Key, T>;

    // Iterators = not stable, references & pointers = stable
    template <typename Key, typename T>
    using StableUnorderedMap = robin_hood::unordered_node_map<Key, T>;
    template <typename Key>
    using StableUnorderedSet = robin_hood::unordered_node_set<Key>;

    // Iterators and references are not stable on insert or erase.
    template <typename Key, typename T>
    using UnorderedMap = ankerl::unordered_dense::map<Key, T>;
    template <typename Key>
    using UnorderedSet = ankerl::unordered_dense::set<Key>;

    template <typename T>
    using Vector = eastl::vector<T>;
    template <typename T, Size N = 1Ui64>
    using Array = eastl::array<T, N>;

    template <typename T, Size N>
    [[nodiscard]] std::span<T> MakeSpan(Array<T, N>& arr) noexcept
    {
        static_assert(N > 0);
        return std::span{arr.data(), N};
    }

    template <typename T, Size N>
    [[nodiscard]] std::span<T> MakeSpan(T (&arr)[N]) noexcept
    {
        static_assert(N > 0);
        return std::span{arr, N};
    }

    template <typename T, Size N>
    [[nodiscard]] std::span<const T> MakeConstSpan(const Array<T, N>& arr) noexcept
    {
        static_assert(N > 0);
        return std::span{arr.data(), N};
    }

    template <typename T, Size N>
    [[nodiscard]] std::span<const T> MakeConstSpan(const T (&arr)[N]) noexcept
    {
        static_assert(N > 0);
        return std::span{arr, N};
    }

    using Entity = entt::entity;
    constexpr auto NullEntity = entt::null;
    using Registry = entt::registry;

    using Json = nlohmann::json;
    using Guid = xg::Guid;

    using SharedMutex = std::shared_mutex;
    using ReadOnlyLock = std::shared_lock<SharedMutex>;
    using ReadWriteLock = std::unique_lock<SharedMutex>;
    using RecursiveMutex = std::recursive_mutex;
    using RecursiveLock = std::unique_lock<RecursiveMutex>;
    using Mutex = std::mutex;
    using UniqueLock = std::unique_lock<Mutex>;
    template <typename... Mutexes>
    using ScopedLock = std::scoped_lock<Mutexes...>;

    template <typename T>
    using Option = std::optional<T>;

    template <typename T>
    inline Option<T> Some(T&& val)
    {
        return std::make_optional(std::forward<T>(val));
    }

    template <typename T>
    inline Option<T> None()
    {
        return std::nullopt;
    }

    template <typename T>
    inline Option<Ref<T>> Some(T& reference)
    {
        return std::make_optional(std::ref(reference));
    }

    template <typename T>
    inline Option<Ref<T>> Some(const T& reference)
    {
        return std::make_optional(std::cref(reference));
    }

    template <typename T, typename... Args>
        requires(!std::is_array_v<T>)
    Ptr<T> MakePtr(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
        requires std::is_array_v<T>
    Ptr<T> MakePtr(const Size size)
    {
        return std::make_unique<T>(size);
    }

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

    template <typename T>
        requires std::is_enum_v<T>
    constexpr std::string_view ToStringView(const T enumerator)
    {
        return magic_enum::enum_name(enumerator);
    }

    template <typename T>
        requires std::is_enum_v<T>
    constexpr const char* ToCStr(const T enumerator)
    {
        return magic_enum::enum_name(enumerator).data();
    }

    template <typename T>
        requires std::is_enum_v<T>
    constexpr auto ToUnderlying(const T enumerator)
    {
        return static_cast<std::underlying_type_t<std::decay_t<T>>>(enumerator);
    }

    template <typename T>
    consteval auto GetTypeName() noexcept
    {
        constexpr std::string_view funcSignature = __FUNCSIG__;
        constexpr auto offset = funcSignature.find_first_not_of(' ', funcSignature.find_first_of('<') + 1);
        return funcSignature.substr(offset, funcSignature.find_last_of('>') - offset);
    }
} // namespace ig

#define IG_NUMERIC_MIN_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::min()
#define IG_NUMERIC_MAX_OF(X) std::numeric_limits<std::decay_t<decltype(X)>>::max()

#define IG_ENUM_FLAGS(ENUM_TYPE)                                                                                                                  \
    inline constexpr ENUM_TYPE operator|(const ENUM_TYPE lhs, const ENUM_TYPE rhs)                                                                \
    {                                                                                                                                             \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                                                                                 \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) | static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }                                                                                                                                             \
    inline constexpr ENUM_TYPE operator&(const ENUM_TYPE lhs, const ENUM_TYPE rhs)                                                                \
    {                                                                                                                                             \
        static_assert(std::is_enum_v<ENUM_TYPE>);                                                                                                 \
        return static_cast<ENUM_TYPE>(static_cast<std::underlying_type_t<ENUM_TYPE>>(lhs) & static_cast<std::underlying_type_t<ENUM_TYPE>>(rhs)); \
    }
