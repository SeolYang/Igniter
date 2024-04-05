#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <Asset/MaterialLoader.h>

namespace ig
{
    MaterialLoader::MaterialLoader(AssetManager& assetManager)
        : assetManager(assetManager)
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

        CachedAsset<Texture> diffuse{ assetManager.LoadTexture(loadDesc.DiffuseTexGuid) };
        if (!diffuse)
        {
            return MakeFail<Material, EMaterialLoadStatus::FailedLoadDiffuse>();
        }

        return MakeSuccess<Material, EMaterialLoadStatus>(Material{ desc, std::move(diffuse) });
    }

    Result<Material, details::EMakeDefaultMatStatus> MaterialLoader::MakeDefault(const AssetInfo& assetInfo)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<Material, details::EMakeDefaultMatStatus::InvalidAssetInfo>();
        }

        Material::Desc snapshot{ .Info = assetInfo,
                                 .LoadDescriptor = { .DiffuseTexGuid = Guid{ DefaultTextureGuid } } };
        CachedAsset<Texture> defaultEngineTex{ assetManager.LoadTexture(Material::EngineDefault) };
        IG_CHECK(defaultEngineTex);
        return MakeSuccess<Material, details::EMakeDefaultMatStatus>(Material{ snapshot, std::move(defaultEngineTex) });
    }
} // namespace ig