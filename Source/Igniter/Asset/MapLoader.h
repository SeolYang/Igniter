#pragma once
#include <Core/Result.h>
#include <Asset/Map.h>

namespace ig
{
    enum class EMapLoadStatus
    {
        Success,
        InvalidAssetInfo,
        AssetTypeMismatch,
        FileDoesNotExists
    };

    class MapLoader final
    {
        friend class AssetManager;

    public:
        MapLoader() = default;
        MapLoader(const MapLoader&) = delete;
        MapLoader(MapLoader&&) noexcept = delete;
        ~MapLoader() = default;

        MapLoader& operator=(const MapLoader&) = delete;
        MapLoader& operator=(MapLoader&&) noexcept = delete;

    private:
        Result<Map, EMapLoadStatus> Load(const Map::Desc& desc);
    };
}    // namespace ig