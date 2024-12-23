#include "Igniter/Igniter.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/MaterialLoader.h"

namespace ig
{
    MaterialLoader::MaterialLoader(AssetManager& assetManager) : assetManager(assetManager) { }

    Result<Material, EMaterialLoadStatus> MaterialLoader::Load(const Material::Desc& desc)
    {
        const AssetInfo&          assetInfo{desc.Info};
        const Material::LoadDesc& loadDesc{desc.LoadDescriptor};

        if (!assetInfo.IsValid())
        {
            return MakeFail<Material, EMaterialLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Material)
        {
            return MakeFail<Material, EMaterialLoadStatus::AssetTypeMismatch>();
        }

        const ManagedAsset<Texture> diffuse{assetManager.LoadTexture(loadDesc.DiffuseTexGuid)};
        if (!diffuse)
        {
            return MakeFail<Material, EMaterialLoadStatus::FailedLoadDiffuse>();
        }

        return MakeSuccess<Material, EMaterialLoadStatus>(Material{assetManager, desc, diffuse});
    }

    Result<Material, details::EMakeDefaultMatStatus> MaterialLoader::MakeDefault(const AssetInfo& assetInfo)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<Material, details::EMakeDefaultMatStatus::InvalidAssetInfo>();
        }

        const Material::Desc        snapshot{.Info = assetInfo, .LoadDescriptor = {.DiffuseTexGuid = Guid{DefaultTextureGuid}}};
        const ManagedAsset<Texture> defaultEngineTex{assetManager.LoadTexture(Material::EngineDefault)};
        IG_CHECK(defaultEngineTex);
        return MakeSuccess<Material, details::EMakeDefaultMatStatus>(Material{assetManager, snapshot, defaultEngineTex});
    }
} // namespace ig
