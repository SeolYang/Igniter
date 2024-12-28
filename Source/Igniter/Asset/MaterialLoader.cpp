#include "Igniter/Igniter.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/MaterialLoader.h"

namespace ig
{
    MaterialLoader::MaterialLoader(AssetManager& assetManager) : assetManager(assetManager) { }

    Result<MaterialAsset, EMaterialLoadStatus> MaterialLoader::Load(const MaterialAsset::Desc& desc)
    {
        const AssetInfo&          assetInfo{desc.Info};
        const MaterialAsset::LoadDesc& loadDesc{desc.LoadDescriptor};

        if (!assetInfo.IsValid())
        {
            return MakeFail<MaterialAsset, EMaterialLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Material)
        {
            return MakeFail<MaterialAsset, EMaterialLoadStatus::AssetTypeMismatch>();
        }

        const ManagedAsset<Texture> diffuse{assetManager.LoadTexture(loadDesc.DiffuseTexGuid)};
        if (!diffuse)
        {
            return MakeFail<MaterialAsset, EMaterialLoadStatus::FailedLoadDiffuse>();
        }

        return MakeSuccess<MaterialAsset, EMaterialLoadStatus>(MaterialAsset{assetManager, desc, diffuse});
    }

    Result<MaterialAsset, details::EMakeDefaultMatStatus> MaterialLoader::MakeDefault(const AssetInfo& assetInfo)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<MaterialAsset, details::EMakeDefaultMatStatus::InvalidAssetInfo>();
        }

        const MaterialAsset::Desc        snapshot{.Info = assetInfo, .LoadDescriptor = {.DiffuseTexGuid = Guid{DefaultTextureGuid}}};
        const ManagedAsset<Texture> defaultEngineTex{assetManager.LoadTexture(MaterialAsset::EngineDefault)};
        IG_CHECK(defaultEngineTex);
        return MakeSuccess<MaterialAsset, details::EMakeDefaultMatStatus>(MaterialAsset{assetManager, snapshot, defaultEngineTex});
    }
} // namespace ig
