#pragma once
#include <Igniter.h>

namespace ig::details
{
    template <typename VARType>
    void Serialize(nlohmann::json& archive, const VARType& var, const std::string_view dataTypeName, const std::string_view varName)
    {
        if (!archive.contains(dataTypeName))
        {
            archive[dataTypeName] = nlohmann::json{};
        }

        IG_CHECK(!archive[dataTypeName].is_discarded());
        archive[dataTypeName][varName] = var;
    }

    template <typename VARType, typename F>
    void DeSerialize(const nlohmann::json& archive, VARType& var, const std::string_view dataTypeName, const std::string_view varName, F callback, VARType fallback)
    {
        IG_CHECK(!archive.is_discarded());
        bool bFALLBACKRequired = false;
        if (archive.contains(dataTypeName) && archive[dataTypeName].contains(varName))
        {
            bFALLBACKRequired = !callback(archive, var);
        }

        if (bFALLBACKRequired)
        {
            /* #sy_improvement FALLBACK 이 발생 하였음을 어떻게 알리면 좋을까? */
            var = fallback;
        }
    }

    template <typename T>
    concept ConvertibleJsonInternalArray = std::is_same<T, json::array_t>::value;

    template <typename T>
    concept ConvertibleJsonInternalUnsigned = std::is_unsigned_v<T> && std::is_convertible_v<T, json::number_unsigned_t>;

    template <typename T>
    concept ConvertibleJsonInternalFloat = std::is_floating_point_v<T> && std::is_convertible_v<T, json::number_float_t>;

    template <typename T>
    concept ConvertibleJsonInternalBoolean = std::is_same_v<T, json::boolean_t>;

    template <typename T>
    concept ConvertibleJsonInternalString = std::is_convertible_v<T, json::string_t>;

    template <typename T>
    struct JsonInternal;

    template <ConvertibleJsonInternalArray T>
    struct JsonInternal<T>
    {
        using Type = json::array_t;
    };

    template <ConvertibleJsonInternalUnsigned T>
    struct JsonInternal<T>
    {
        using Type = json::number_unsigned_t;
    };

    template <ConvertibleJsonInternalFloat T>
    struct JsonInternal<T>
    {
        using Type = json::number_float_t;
    };

    template <ConvertibleJsonInternalString T>
    struct JsonInternal<T>
    {
        using Type = json::string_t;
    };

    template <typename T>
    using JsonInternal_t = JsonInternal<T>::Type;
} // namespace ig::details

#define IG_SERIALIZE_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR) \
    ig::details::Serialize(JSON_ARCHIVE, DATA.VAR, #DATA_TYPE, #VAR)

#define IG_SERIALIZE_GUID_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR) \
    ig::details::Serialize(JSON_ARCHIVE, DATA.VAR.str(), #DATA_TYPE, #VAR)

#define IG_SERIALIZE_ENUM_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR) \
    ig::details::Serialize(JSON_ARCHIVE, magic_enum::enum_name<std::decay_t<decltype(DATA.VAR)>>(DATA.VAR), #DATA_TYPE, #VAR)

#define IG_SERIALIZE_STRING_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR) \
    ig::details::Serialize(JSON_ARCHIVE, DATA.VAR.ToStringView(), #DATA_TYPE, #VAR)

/*
 * #sy_log Deserialize 하고 싶은 Json Archive 내부에 원하는 값이 없을 수도 있다. 그에 대해선 아래와 같이 대응 하기로 결정.
 * 1. 값 설정 Callback 은 Json 내부에 원하는 값이 있는 경우에만 호출 된다.
 *  1.1. 단 이 경우, '키'는 유효하지만 원하는 타입으로 가져올 수 없는 경우엔 Callback 함수는 false 를 반환해야 한다.
 *  1.2. 만약 값을 가져오는데 완전히 성공하면 true 를 반환해야 한다.
 * 2. 만약 원하는 값이 없는 경우이거나 (1.1)의 경우 대신 지정한 Fall-back 값이 설정 되도록 해야 한다.
 *
 * 이렇게, Deserialize 과정에서 실제 값이 Json 에 없는 경우에 대한 오류 처리를 유연하게 할 수 있다.
 */
#define IG_DESERIALIZE_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR, FALLBACK)                                           \
    ig::details::DeSerialize<std::decay_t<decltype(DATA.VAR)>>(                                                     \
        JSON_ARCHIVE, DATA.VAR, #DATA_TYPE, #VAR,                                                                   \
        [](const nlohmann::json& archive, std::decay_t<decltype(DATA.VAR)>& var) {                                  \
            IG_CHECK(archive.contains(#DATA_TYPE) && archive[#DATA_TYPE].contains(#VAR));                           \
            using JsonDATA_TYPE_t = ig::details::JsonInternal_t<std::decay_t<decltype(var)>>;                       \
            const JsonDATA_TYPE_t* valuePtr = archive[#DATA_TYPE][#VAR].template get_ptr<const JsonDATA_TYPE_t*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                         \
            if (bTypeMismatch)                                                                                      \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
            var = static_cast<std::decay_t<decltype(DATA.VAR)>>(*valuePtr);                                         \
            return true;                                                                                            \
        },                                                                                                          \
        FALLBACK)

#define IG_DESERIALIZE_GUID_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR, FALLBACK)                              \
    ig::details::DeSerialize<xg::Guid>(                                                                     \
        JSON_ARCHIVE, DATA.VAR, #DATA_TYPE, #VAR,                                                           \
        [](const nlohmann::json& archive, xg::Guid& var) {                                                  \
            IG_CHECK(archive.contains(#DATA_TYPE) && archive[#DATA_TYPE].contains(#VAR));                           \
            const std::string* valuePtr = archive[#DATA_TYPE][#VAR].template get_ptr<const std::string*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                 \
            if (bTypeMismatch)                                                                              \
            {                                                                                               \
                return false;                                                                               \
            }                                                                                               \
            var = xg::Guid(valuePtr->c_str());                                                              \
            return true;                                                                                    \
        },                                                                                                  \
        FALLBACK)

#define IG_DESERIALIZE_ENUM_JSON(DATA_TYPE, JSON_ARCHIVE, DATA, VAR, FALLBACK)                              \
    ig::details::DeSerialize<std::decay_t<decltype(DATA.VAR)>>(                                             \
        JSON_ARCHIVE, DATA.VAR, #DATA_TYPE, #VAR,                                                           \
        [](const nlohmann::json& archive, std::decay_t<decltype(DATA.VAR)>& var) {                          \
            IG_CHECK(archive.contains(#DATA_TYPE) && archive[#DATA_TYPE].contains(#VAR));                           \
            const std::string* valuePtr = archive[#DATA_TYPE][#VAR].template get_ptr<const std::string*>(); \
            const bool bTypeMismatch = valuePtr == nullptr;                                                 \
            if (bTypeMismatch)                                                                              \
            {                                                                                               \
                return false;                                                                               \
            }                                                                                               \
            auto valueOpt = magic_enum::enum_cast<std::decay_t<decltype(DATA.VAR)>>(*valuePtr);             \
            if (!valueOpt)                                                                                  \
            {                                                                                               \
                return false;                                                                               \
            }                                                                                               \
            var = *valueOpt;                                                                                \
            return true;                                                                                    \
        },                                                                                                  \
        FALLBACK)