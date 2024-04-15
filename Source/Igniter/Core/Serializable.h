#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename Archive, typename Data>
    concept Serializable = requires(Archive& archive, const Data& data)
    {
        {
            data.Serialize(archive)
        } -> std::convertible_to<Archive&>;
    };

    template <typename Archive, typename Data>
    concept Deserializable = requires(const Archive& archive, Data& data)
    {
        {
            data.Deserialize(archive)
        } -> std::convertible_to<const Archive&>;
    };

    template <typename Archive, typename Data>
        requires Serializable<Archive, Data>
    Archive& operator<<(Archive& archive, const Data& data)
    {
        return data.Serialize(archive);
    }

    template <typename Archive, typename Data>
        requires Deserializable<Archive, Data>
    const Archive& operator>>(const Archive& archive, Data& data)
    {
        return data.Deserialize(archive);
    }
} // namespace ig
