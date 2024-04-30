#include <Igniter.h>
#include <Core/Serialization.h>
#include <Filesystem/Utils.h>
#include <Gameplay/World.h>
#include <Asset/MapCreator.h>

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

        Json serializedWorld{};
        if (desc.WorldToSerialize != nullptr)
        {
            desc.WorldToSerialize->Serialize(serializedWorld);
        }

        Json serializedMeta{};
        serializedMeta << assetInfo;

        const Path metadataPath{MakeAssetMetadataPath(EAssetCategory::Map, assetInfo.GetGuid())};
        IG_CHECK(!metadataPath.empty());
        if (!SaveJsonToFile(metadataPath, serializedMeta))
        {
            return MakeFail<Map::Desc, EMapCreateStatus::FailedSaveMetadata>();
        }

        const Path assetPath{MakeAssetPath(EAssetCategory::Map, assetInfo.GetGuid())};

        if (!fs::exists(assetPath) && !SaveJsonToFile(assetPath, serializedWorld))
        {
            return MakeFail<Map::Desc, EMapCreateStatus::FailedSaveAsset>();
        }

        return MakeSuccess<Map::Desc, EMapCreateStatus>(Map::Desc{.Info = assetInfo});
    }
}    // namespace ig