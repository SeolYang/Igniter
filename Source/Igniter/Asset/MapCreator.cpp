#include "Igniter/Igniter.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Asset/MapCreator.h"

namespace ig
{
    Result<Map::Desc, EMapCreateStatus> MapCreator::Import(const AssetInfo& assetInfo, const MapCreateDesc& desc)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<Map::Desc, EMapCreateStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Map)
        {
            return MakeFail<Map::Desc, EMapCreateStatus::InvalidAssetType>();
        }

        Json serializedWorld{ };
        if (desc.WorldToSerialize != nullptr)
        {
            desc.WorldToSerialize->Serialize(serializedWorld);
        }

        Json serializedMeta{ };
        serializedMeta << assetInfo;

        const Path metadataPath{MakeAssetMetadataPath(EAssetCategory::Map, assetInfo.GetGuid())};
        IG_CHECK(!metadataPath.empty());
        if (!SaveJsonToFile(metadataPath, serializedMeta))
        {
            return MakeFail<Map::Desc, EMapCreateStatus::FailedSaveMetadata>();
        }

        const Path assetPath{MakeAssetPath(EAssetCategory::Map, assetInfo.GetGuid())};

        if (!fs::exists(assetPath) && !SaveBlobToFile(assetPath, Json::to_ubjson(serializedWorld)))
        {
            return MakeFail<Map::Desc, EMapCreateStatus::FailedSaveAsset>();
        }

        return MakeSuccess<Map::Desc, EMapCreateStatus>(Map::Desc{.Info = assetInfo});
    }
} // namespace ig
