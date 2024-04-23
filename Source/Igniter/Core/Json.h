#pragma once
#include <Igniter.h>
#include <Core/Log.h>

IG_DEFINE_LOG_CATEGORY(JsonDeserializer);

namespace ig::details
{
    template <typename VarType>
    void Serialize(Json& archive, const VarType& var, const std::string_view containerKey, const std::string_view varKey)
    {
        if (!archive.contains(containerKey))
        {
            archive[containerKey] = nlohmann::json{};
        }

        IG_CHECK(!archive[containerKey].is_discarded());
        archive[containerKey][varKey] = var;
    }

    template <typename VarType, typename F>
    void Deserialize(
        const Json& archive, VarType& var, const std::string_view containerKey, const std::string_view varKey, F callback, VarType fallback)
    {
        IG_CHECK(!archive.is_discarded());
        if (!archive.contains(containerKey) || !archive[containerKey].contains(varKey))
        {
            var = fallback;
            IG_LOG(JsonDeserializer, Warning, "{}::{} does not exists in Json archive.", containerKey, varKey);
            return;
        }

        if (!callback(archive, var))
        {
            var = fallback;
            IG_LOG(JsonDeserializer, Warning, "Type mismatch/Invalid value found in {}::{}.", containerKey, varKey);
            return;
        }
    }

    template <typename T>
    concept ConvertibleJsonInternalArray = std::is_same<T, Json::array_t>::value;

    template <typename T>
    concept ConvertibleJsonInternalUnsigned = !std::is_same_v<T, bool> && std::is_unsigned_v<T> && std::is_convertible_v<T, Json::number_unsigned_t>;

    template <typename T>
    concept ConvertibleJsonInternalFloat = std::is_floating_point_v<T> && std::is_convertible_v<T, Json::number_float_t>;

    template <typename T>
    concept ConvertibleJsonInternalBoolean = std::is_same_v<T, Json::boolean_t>;

    template <typename T>
    concept ConvertibleJsonInternalString = std::is_convertible_v<T, Json::string_t>;

    template <typename T>
    struct JsonInternal;

    template <ConvertibleJsonInternalArray T>
    struct JsonInternal<T>
    {
        using Type = Json::array_t;
    };

    template <ConvertibleJsonInternalUnsigned T>
    struct JsonInternal<T>
    {
        using Type = Json::number_unsigned_t;
    };

    template <ConvertibleJsonInternalFloat T>
    struct JsonInternal<T>
    {
        using Type = Json::number_float_t;
    };

    template <ConvertibleJsonInternalString T>
    struct JsonInternal<T>
    {
        using Type = Json::string_t;
    };

    template <ConvertibleJsonInternalBoolean T>
    struct JsonInternal<T>
    {
        using Type = Json::boolean_t;
    };

    template <typename T>
    using JsonInternal_t = JsonInternal<T>::Type;
}    // namespace ig::details

#define IG_SERIALIZE_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY) ig::details::Serialize(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY)

#define IG_SERIALIZE_GUID_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY) ig::details::Serialize(JSON_ARCHIVE, VAR.str(), CONTAINER_KEY, VALUE_KEY)

#define IG_SERIALIZE_ENUM_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY) \
    ig::details::Serialize(JSON_ARCHIVE, magic_enum::enum_name<std::decay_t<decltype(VAR)>>(VAR), CONTAINER_KEY, VALUE_KEY)

#define IG_SERIALIZE_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR) IG_SERIALIZE_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR)

#define IG_SERIALIZE_GUID_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR) IG_SERIALIZE_GUID_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR)

#define IG_SERIALIZE_ENUM_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR) IG_SERIALIZE_ENUM_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR)

/*
 * #sy_log Deserialize 하고 싶은 Json Archive 내부에 원하는 값이 없을 수도 있다. 그에 대해선 아래와 같이 대응 하기로 결정.
 * 1. 값 설정 Callback 은 Json 내부에 원하는 값이 있는 경우에만 호출 된다.
 *  1.1. 단 이 경우, '키'는 유효하지만 원하는 타입으로 가져올 수 없는 경우엔 Callback 함수는 false 를 반환해야 한다.
 *  1.2. 만약 값을 가져오는데 완전히 성공하면 true 를 반환해야 한다.
 * 2. 만약 원하는 값이 없는 경우이거나 (1.1)의 경우 대신 지정한 Fall-back 값이 설정 되도록 해야 한다.
 *
 * 이렇게, Deserialize 과정에서 실제 값이 Json 에 없는 경우에 대한 오류 처리를 유연하게 할 수 있다.
 */

#define IG_DESERIALIZE_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY, FALLBACK)                                  \
    ig::details::Deserialize<std::decay_t<decltype(VAR)>>(                                                          \
        JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY,                                                                \
        [](const ig::Json& archive, std::decay_t<decltype(VAR)>& var)                                               \
        {                                                                                                           \
            IG_CHECK(archive.contains(CONTAINER_KEY) && archive[CONTAINER_KEY].contains(VALUE_KEY));                \
            using JsoInternal = ig::details::JsonInternal_t<std::decay_t<decltype(var)>>;                           \
            const JsoInternal* valuePtr = archive[CONTAINER_KEY][VALUE_KEY].template get_ptr<const JsoInternal*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                         \
            if (bTypeMismatch)                                                                                      \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
            var = static_cast<std::decay_t<decltype(VAR)>>(*valuePtr);                                              \
            return true;                                                                                            \
        },                                                                                                          \
        FALLBACK)

#define IG_DESERIALIZE_GUID_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY, FALLBACK)                             \
    ig::details::Deserialize<ig::Guid>(                                                                             \
        JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY,                                                                \
        [](const ig::Json& archive, ig::Guid& var)                                                                  \
        {                                                                                                           \
            IG_CHECK(archive.contains(CONTAINER_KEY) && archive[CONTAINER_KEY].contains(VALUE_KEY));                \
            const std::string* valuePtr = archive[CONTAINER_KEY][VALUE_KEY].template get_ptr<const std::string*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                         \
            if (bTypeMismatch)                                                                                      \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
            var = xg::Guid(valuePtr->c_str());                                                                      \
            return true;                                                                                            \
        },                                                                                                          \
        FALLBACK)

#define IG_DESERIALIZE_ENUM_JSON(JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY, FALLBACK)                             \
    ig::details::Deserialize<std::decay_t<decltype(VAR)>>(                                                          \
        JSON_ARCHIVE, VAR, CONTAINER_KEY, VALUE_KEY,                                                                \
        [](const ig::Json& archive, std::decay_t<decltype(VAR)>& var)                                               \
        {                                                                                                           \
            IG_CHECK(archive.contains(CONTAINER_KEY) && archive[CONTAINER_KEY].contains(VALUE_KEY));                \
            const std::string* valuePtr = archive[CONTAINER_KEY][VALUE_KEY].template get_ptr<const std::string*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                         \
            if (bTypeMismatch)                                                                                      \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
            auto valueOpt = magic_enum::enum_cast<std::decay_t<decltype(VAR)>>(*valuePtr);                          \
            if (!valueOpt)                                                                                          \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
            var = *valueOpt;                                                                                        \
            return true;                                                                                            \
        },                                                                                                          \
        FALLBACK)

#define IG_DESERIALIZE_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR, FALLBACK) IG_DESERIALIZE_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR, FALLBACK)

#define IG_DESERIALIZE_GUID_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR, FALLBACK) \
    IG_DESERIALIZE_GUID_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR, FALLBACK)

#define IG_DESERIALIZE_ENUM_JSON_SIMPLE(DATA_TYPE, JSON_ARCHIVE, VAR, FALLBACK) \
    IG_DESERIALIZE_ENUM_JSON(JSON_ARCHIVE, VAR, #DATA_TYPE, #VAR, FALLBACK)
