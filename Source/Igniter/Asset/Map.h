#pragma once
#include "Igniter/Asset/Common.h"

namespace ig
{
    class World;

    struct MapCreateDesc final
    {
    public:
        World* WorldToSerialize = nullptr;
    };

    struct MapLoadDesc final
    {
    public:
        Json& Serialize(Json& archive) const { return archive; }
        const Json& Deserialize(const Json& archive) { return archive; }
    };

    class Map final
    {
    public:
        using ImportDesc = MapCreateDesc;
        using LoadDesc = MapLoadDesc;
        using Desc = AssetDesc<Map>;

    public:
        Map(const Desc& snapshot, Json serializedWorld)
            : snapshot(snapshot)
            , serializedWorld(serializedWorld)
        {}

        Map(const Map&) = delete;
        Map(Map&&) noexcept = default;
        ~Map() = default;

        Map& operator=(const Map&) = delete;
        Map& operator=(Map&&) noexcept = default;

        [[nodiscard]] const Desc& GetSnapshot() const { return snapshot; }
        [[nodiscard]] const Json& GetSerializedWorld() const { return serializedWorld; }

    private:
        Desc snapshot{};
        Json serializedWorld{};
    };
} // namespace ig
