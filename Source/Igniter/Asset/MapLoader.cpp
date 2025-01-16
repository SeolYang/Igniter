#include "Igniter/Igniter.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Asset/MapLoader.h"

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

        /* #sy_todo ubjson 으로 저장할지, 아니면 그냥 json으로 저장할지 load desc 에서 설정 할 수 있도록 할 것! */
        const Vector<U8> blob = LoadBlobFromFile(assetPath);
        return MakeSuccess<Map, EMapLoadStatus>(Map{desc, Json::from_ubjson(std::span{blob.data(), blob.size()})});
    }
} // namespace ig
