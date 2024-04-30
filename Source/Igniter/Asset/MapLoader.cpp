#include <Igniter.h>
#include <Filesystem/Utils.h>
#include <Asset/MapLoader.h>

namespace ig
{
    Result<Map, EMapLoadStatus> MapLoader::Load(const Map::Desc& desc)
    {
        const AssetInfo& assetInfo{desc.Info};
        if (!assetInfo.IsValid())
        {
            return MakeFail<Map, EMapLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Map)
        {
            return MakeFail<Map, EMapLoadStatus::AssetTypeMismatch>();
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::Map, assetInfo.GetGuid());
        if (!fs::exists(assetPath))
        {
            return MakeFail<Map, EMapLoadStatus::FileDoesNotExists>();
        }

        return MakeSuccess<Map, EMapLoadStatus>(Map{desc, Json::from_ubjson(LoadBlobFromFile(assetPath))});
    }
}    // namespace ig
