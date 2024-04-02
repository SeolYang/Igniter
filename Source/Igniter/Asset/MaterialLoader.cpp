#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <Asset/MaterialLoader.h>

namespace ig
{
    MaterialLoader::MaterialLoader(AssetManager& assetManager) : assetManager(assetManager)
    {
    }

    Result<Material, EMaterialLoadStatus> MaterialLoader::Load(const Material::Desc& desc)
    {
        const AssetInfo& assetInfo{ desc.Info };
        const Material::LoadDesc& loadDesc{ desc.LoadDescriptor };

        if (!assetInfo.IsValid())
        {
            return MakeFail<Material, EMaterialLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetType() != EAssetType::Material)
        {
            return MakeFail<Material, EMaterialLoadStatus::AssetTypeMismatch>();
        }

        if (!loadDesc.DiffuseVirtualPath.IsValid())
        {
            return MakeFail<Material, EMaterialLoadStatus::InvalidDiffuseVirtualPath>();
        }

        CachedAsset<Texture> diffuse{ assetManager.LoadTexture(loadDesc.DiffuseVirtualPath) };
        if (!diffuse)
        {
            return MakeFail<Material, EMaterialLoadStatus::FailedLoadDiffuse>();
        }

        return MakeSuccess<Material, EMaterialLoadStatus>(Material{ desc, std::move(diffuse) });
    }
} // namespace ig