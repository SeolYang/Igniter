#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename Archive, typename Ty>
    concept Serializable = requires(Archive& archive, const Ty& data) {
        {
            data.Serialize(archive)
        } -> std::same_as<Archive&>;
    };

    template <typename Archive, typename Ty>
    concept Deserializable = requires(const Archive& archive, Ty& data) {
        {
            data.Deserialize(archive)
        } -> std::same_as<const Archive&>;
    };

    template <typename Archive, typename Ty>
        requires Serializable<Archive, Ty>
    Archive& Serialize(Archive& archive, const Ty& data)
    {
        return data.Serialize(archive);
    }

    template <typename Archive, typename Ty>
        requires Deserializable<Archive, Ty>
    const Archive& Deserialize(const Archive& archive, Ty& data)
    {
        return data.Deserialize(archive);
    }

    template <typename Archive, typename Ty>
        requires Serializable<Archive, Ty>
    Archive& operator<<(Archive& archive, const Ty& data)
    {
        return data.Serialize(archive);
    }

    template <typename Archive, typename Ty>
        requires Deserializable<Archive, Ty>
    const Archive& operator>>(const Archive& archive, Ty& data)
    {
        return data.Deserialize(archive);
    }

    // 정적으로 타입을 알 수 없는 경우 사용 할 것!
    template <typename Archive, typename Ty>
        requires Serializable<Archive, Ty>
    void SerializeTypeless(Archive* archive, const void* dataPtr)
    {
        if (archive == nullptr || dataPtr == nullptr)
        {
            return;
        }

        reinterpret_cast<const Ty*>(dataPtr)->Serialize(*archive);
    }

    template <typename Archive, typename Ty>
        requires Deserializable<Archive, Ty>
    void DeserializeTypeless(const Archive* archive, void* dataPtr)
    {
        if (archive == nullptr || dataPtr == nullptr)
        {
            return;
        }

        reinterpret_cast<Ty*>(dataPtr)->Deserialize(*archive);
    }

    constexpr inline entt::hashed_string SerializeJsonFunc = "SerializeJson"_hs;
    constexpr inline entt::hashed_string DeserializeJsonFunc = "DeserializeJson"_hs;
}    // namespace ig

#define IG_SET_META_SERIALIZE_JSON(T) entt::meta<T>().template func<&ig::SerializeTypeless<ig::Json, T>>(ig::SerializeJsonFunc)
#define IG_SET_META_DESERIALIZE_JSON(T) entt::meta<T>().template func<&ig::DeserializeTypeless<ig::Json, T>>(ig::DeserializeJsonFunc)

// #sy_note 지원 타입 더 늘리기 위해서는 Serialize 할 타입에 대해 특정 Archive에 대한 함수를 오버로딩 해주고. DefineMeta 에서 자유(독립) 함수를
// 등록 해주면
