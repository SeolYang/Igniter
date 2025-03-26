#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/Core/Serialization.h"

namespace ig::details
{
    template <typename T>
    concept ConvertibleJsonInternalArray = std::is_same<T, Json::array_t>::value;

    template <typename T>
    concept ConvertibleJsonInternalSigned =
        !std::is_floating_point_v<T> && !std::is_same_v<T, bool> && std::is_signed_v<T> && std::is_convertible_v<T, Json::number_unsigned_t>;

    template <typename T>
    concept ConvertibleJsonInternalUnsigned =
        !std::is_floating_point_v<T> && !std::is_same_v<T, bool> && std::is_unsigned_v<T> && std::is_convertible_v<T, Json::number_unsigned_t>;

    template <typename T>
    concept ConvertibleJsonInternalFloat = std::is_floating_point_v<T> && std::is_convertible_v<T, Json::number_float_t>;

    template <typename T>
    concept ConvertibleJsonInternalBoolean = std::is_same_v<T, Json::boolean_t>;

    template <typename T>
    concept ConvertibleJsonInternalString = std::is_convertible_v<T, Json::string_t>;

    template <typename T>
    struct JsonInternal
    {
        using Type = void;
    };

    template <ConvertibleJsonInternalArray T>
    struct JsonInternal<T>
    {
        using Type = Json::array_t;
    };

    template <ConvertibleJsonInternalSigned T>
    struct JsonInternal<T>
    {
        using Type = Json::number_integer_t;
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
} // namespace ig::details

namespace ig
{
    template <typename T>
    Json ToJson(const T& data)
    {
        Json newJson{};
        data.Serialize(newJson);
        return newJson;
    }

    /*
     * Serializable 타입에 대한 FromJson 여부는 다소 모호 할 수 있음.
     * 완전 성공을 성공으로 보느냐, 부분 성공도 성공으로 보느냐를 정확히 하기 어려움.
     * 그러므로, 전달된 json 객체가 object 타입 또는 null이 아닌 경우 실패 하였다고 보고.
     * 실제로 json 객체를 통해 값을 완벽하게 Deserialize 하지 못하였다 하더라도,
     * 데이터 타입의 deserialize가 적절한 fallback 처리를 해주었다고 가정 하고,
     * 내부의 값 자체는 정상적으로 처리 되었기 때문에 성공으로 가정.
     * 예시. 빈 json object를 Deserialize 하였을 때, Data 내부를 fallback value로 채워 넣는 것은 정상적인 작동 방식이다.
     */
    template <typename T>
    bool FromJson(const Json& json, T& data)
    {
        if (!json.is_object() && !json.is_null())
        {
            return false;
        }

        data.Deserialize(json);
        return true;
    }

    template <typename T>
        requires(!std::is_void_v<details::JsonInternal_t<T>>)
    Json ToJson(const T& data)
    {
        return data;
    }

    template <typename T>
        requires(!std::is_void_v<details::JsonInternal_t<T>>)
    bool FromJson(const Json& json, T& data)
    {
        nlohmann::from_json(json, data);
        const typename details::JsonInternal_t<T>* serializedPtr =
            json.template get_ptr<const typename details::JsonInternal_t<T>*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        data = static_cast<T>(*serializedPtr);
        return true;
    }

    template <>
    inline Json ToJson(const Guid& guid)
    {
        Json newJson;
        nlohmann::to_json(newJson, guid.str());
        return newJson;
    }

    template <>
    inline bool FromJson(const Json& json, Guid& guid)
    {
        const std::string* serializedPtr = json.template get_ptr<const std::string*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        guid = Guid{*serializedPtr};
        return true;
    }

    template <>
    inline Json ToJson(const String& str)
    {
        return ToJson(str.ToStandard());
    }

    template <>
    inline bool FromJson(const Json& json, String& str)
    {
        const std::string* serializedPtr = json.template get_ptr<const std::string*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        str = String(*serializedPtr);
        return true;
    }

    template <typename E>
        requires std::is_enum_v<E>
    Json ToJson(const E& enumerator)
    {
        Json newJson{};
        nlohmann::to_json(newJson, magic_enum::enum_name<E>(enumerator));
        return newJson;
    }

    template <typename E>
        requires std::is_enum_v<E>
    bool FromJson(const Json& json, E& enumerator)
    {
        enumerator = magic_enum::enum_value<E>(0);

        const std::string* serializedPtr = json.template get_ptr<const std::string*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        std::optional<E> enumOpt = magic_enum::enum_cast<E>(*serializedPtr);
        if (!enumOpt)
        {
            return false;
        }

        enumerator = *enumOpt;
        return true;
    }

    template <typename T, Size N>
    Json ToJson(const Array<T, N>& array)
    {
        Json newJson{};

        if constexpr (!std::is_void_v<details::JsonInternal_t<T>>)
        {
            nlohmann::to_json(newJson, std::vector<T>{array.cbegin(), array.cend()});
        }
        else
        {
            std::vector<Json> newJsonVector;
            newJsonVector.resize(N);
            for (Index idx = 0; idx < N; ++idx)
            {
                newJsonVector[idx] = ToJson(array[idx]);
            }
            nlohmann::to_json(newJson, newJsonVector);
        }

        return newJson;
    }

    template <typename T, Size N>
    bool FromJson(const Json& json, Array<T, N>& array)
    {
        const std::vector<Json>* serializedPtr = json.template get_ptr<const std::vector<Json>*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        const ig::Size minSize = std::min(N, serializedPtr->size());
        for (ig::Index idx = 0; idx < minSize; ++idx)
        {
            if (!FromJson(serializedPtr->at(idx), array[idx]))
            {
                return false;
            }
        }

        return true;
    }

    template <typename T>
    Json ToJson(const Vector<T>& vector)
    {
        Json newJson{};

        if constexpr (!std::is_void_v<details::JsonInternal_t<T>>)
        {
            nlohmann::to_json(newJson, vector);
        }
        else
        {
            const Size numData = vector.size();
            std::vector<Json> newJsonVector;
            newJsonVector.resize(numData);
            for (Index idx = 0; idx < numData; ++idx)
            {
                newJsonVector[idx] = ToJson(vector[idx]);
            }
            nlohmann::to_json(newJson, newJsonVector);
        }

        return newJson;
    }

    template <typename T>
    bool FromJson(const Json& json, Vector<T>& vector)
    {
        const std::vector<Json>* serializedPtr = json.template get_ptr<const std::vector<Json>*>();
        if (serializedPtr == nullptr)
        {
            return false;
        }

        const Size numSerializedData = serializedPtr->size();
        vector.resize(numSerializedData);
        for (ig::Index idx = 0; idx < numSerializedData; ++idx)
        {
            if (!FromJson(serializedPtr->at(idx), vector[idx]))
            {
                return false;
            }
        }

        return true;
    }
} // namespace ig

#define IG_SERIALIZE_TO_JSON(CLASS, ARCHIVE, VARIABLE) ARCHIVE[#CLASS][#VARIABLE] = ig::ToJson(VARIABLE)
#define IG_SERIALIZE_TO_JSON_EXPR(CLASS, ARCHIVE, VARIABLE, EXPRESSION) \
    static_assert(!std::is_void_v<decltype(CLASS::VARIABLE)>); \
    ARCHIVE[#CLASS][#VARIABLE] = ig::ToJson(EXPRESSION)

/* FromJson 실패 시 Variable Type의 Default Constructor를 Fallback 값으로 사용 */
#define IG_DESERIALIZE_FROM_JSON_TEMP(CLASS, ARCHIVE, VARIABLE, TEMP)    \
    if (!ARCHIVE.contains(#CLASS) ||                          \
        !ARCHIVE[#CLASS].contains(#VARIABLE) ||               \
        !ig::FromJson(ARCHIVE[#CLASS][#VARIABLE], VARIABLE))  \
    {                                                         \
        TEMP = std::remove_cvref_t<decltype(TEMP)>{}; \
    }

/* FromJson 실패 시 지정된 Fallback 값을 사용. */
#define IG_DESERIALIZE_FROM_JSON_TEMP_FALLBACK(CLASS, ARCHIVE, VARIABLE, TEMP, FALLBACK) \
    if (!ARCHIVE.contains(#CLASS) ||                                          \
        !ARCHIVE[#CLASS].contains(#VARIABLE) ||                               \
        !ig::FromJson(ARCHIVE[#CLASS][#VARIABLE], TEMP))                  \
    {                                                                         \
        TEMP = FALLBACK;                                                  \
    }

/* FromJson 실패 시 기존 값 유지. 단 Vector와 Array는 최초 실패 지점 전 까지의 원소가 반영 됨. */
#define IG_DESERIALIZE_FROM_JSON_TEMP_NO_FALLBACK(CLASS, ARCHIVE, VARIABLE, TEMP) \
    if (ARCHIVE.contains(#CLASS) &&                                    \
        ARCHIVE[#CLASS].contains(#VARIABLE))                           \
    {                                                                  \
        ig::FromJson(ARCHIVE[#CLASS][#VARIABLE], TEMP);            \
    }

/* FromJson 실패 시 Variable Type의 Default Constructor를 Fallback 값으로 사용 */
#define IG_DESERIALIZE_FROM_JSON(CLASS, ARCHIVE, VARIABLE)    \
    IG_DESERIALIZE_FROM_JSON_TEMP(CLASS, ARCHIVE, VARIABLE, VARIABLE)

/* FromJson 실패 시 지정된 Fallback 값을 사용. */
#define IG_DESERIALIZE_FROM_JSON_FALLBACK(CLASS, ARCHIVE, VARIABLE, FALLBACK) \
    IG_DESERIALIZE_FROM_JSON_TEMP_FALLBACK(CLASS, ARCHIVE, VARIABLE, VARIABLE, FALLBACK)

/* FromJson 실패 시 기존 값 유지. 단 Vector와 Array는 최초 실패 지점 전 까지의 원소가 반영 됨. */
#define IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(CLASS, ARCHIVE, VARIABLE) \
    IG_DESERIALIZE_FROM_JSON_TEMP_NO_FALLBACK(CLASS, ARCHIVE, VARIABLE, VARIABLE)
