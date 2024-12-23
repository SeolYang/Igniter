#pragma once
#include "Igniter/Core/Result.h"
#include "Igniter/Asset/Map.h"

namespace ig
{
    enum class EMapCreateStatus
    {
        Success,
        InvalidAssetInfo,
        InvalidAssetType,
        FailedSaveMetadata,
        FailedSaveAsset
    };

    class MapCreator final
    {
        friend class AssetManager;

    public:
        MapCreator()                      = default;
        MapCreator(const MapCreator&)     = delete;
        MapCreator(MapCreator&&) noexcept = delete;
        ~MapCreator()                     = default;

        MapCreator& operator=(const MapCreator&)     = delete;
        MapCreator& operator=(MapCreator&&) noexcept = delete;

    private:
        Result<Map::Desc, EMapCreateStatus> Import(const AssetInfo& assetInfo, const MapCreateDesc& desc);
    };
} // namespace ig
