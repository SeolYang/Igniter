#pragma once
#include "Igniter/Igniter.h"

namespace ig::meta
{
    constexpr inline entt::hashed_string SerializeComponentJsonFunc = "SerializeComponentJson"_hs;
    constexpr inline entt::hashed_string DeserializeComponentJsonFunc = "DeserializeComponentJson"_hs;
} // namespace ig::meta

namespace ig
{
    template <typename Archive, typename Ty>
    concept SelfSerializable = requires(Archive& archive, const Archive& constArchive, Ty& data, const Ty& constData)
    {
        { constData.Serialize(archive) } -> std::same_as<Archive&>;
        { data.Deserialize(constArchive) } -> std::same_as<const Archive&>;
    };

    template <typename Archive, typename Ty>
    Archive& Serialize(Archive& archive, const Ty& data);

    template <typename Archive, typename Ty>
    const Archive& Deserialize(const Archive& archive, Ty& data);

    template <typename Archive, typename Ty>
        requires SelfSerializable<Archive, Ty>
    Archive& Serialize(Archive& archive, const Ty& data)
    {
        return data.Serialize(archive);
    }

    template <typename Archive, typename Ty>
        requires SelfSerializable<Archive, Ty>
    const Archive& Deserialize(const Archive& archive, Ty& data)
    {
        return data.Deserialize(archive);
    }

    template <typename Archive, typename Ty>
        requires SelfSerializable<Archive, Ty>
    Archive& operator<<(Archive& archive, const Ty& data)
    {
        return data.Serialize(archive);
    }

    template <typename Archive, typename Ty>
        requires SelfSerializable<Archive, Ty>
    const Archive& operator>>(const Archive& archive, Ty& data)
    {
        return data.Deserialize(archive);
    }

    template <typename Archive, typename Ty>
    void SerializeComponent(Ref<Archive> archiveRef, Ref<const Registry> registryRef, const Entity entity)
    {
        const Registry& registry = registryRef.get();
        if (registry.all_of<Ty>(entity))
        {
            const Ty& component = registry.get<Ty>(entity);
            Serialize<Archive, Ty>(archiveRef.get(), component);
        }
    }

    template <typename Archive, typename Ty>
    void DeserializeComponent(Ref<const Archive> archiveRef, Ref<Registry> registryRef, const Entity entity)
    {
        Registry& registry = registryRef.get();
        if (registry.all_of<Ty>(entity))
        {
            Ty& component = registry.get<Ty>(entity);
            Deserialize<Archive, Ty>(archiveRef.get(), component);
        }
    }
} // namespace ig

#define IG_SET_META_JSON_SERIALIZABLE_COMPONENT(T)                                                             \
    entt::meta<T>().template func<&ig::SerializeComponent<ig::Json, T>>(ig::meta::SerializeComponentJsonFunc); \
    entt::meta<T>().template func<&ig::DeserializeComponent<ig::Json, T>>(ig::meta::DeserializeComponentJsonFunc);
