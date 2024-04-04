#include <Igniter.h>
#include <Core/Log.h>
#include <Filesystem/Utils.h>
#include <Asset/AssetCache.h>
#include <Asset/AssetManager.h>
#include <Asset/MaterialImporter.h>

IG_DEFINE_LOG_CATEGORY(MaterialImporter);

namespace ig
{
    MaterialImporter::MaterialImporter(AssetManager& assetManager)
        : assetManager(assetManager)
    {
    }

    Result<Material::Desc, EMaterialCreateStatus> MaterialImporter::Create(const AssetInfo& assetInfo, MaterialCreateDesc desc)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetType() != EAssetType::Material)
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::InvalidAssetType>();
        }

        CachedAsset<Texture> diffuse{ assetManager.LoadTexture(desc.DiffuseVirtualPath) };
        if (!diffuse)
        {
            IG_LOG(MaterialImporter, Warning, "Failed to load texture {} to create material {}.", desc.DiffuseVirtualPath, assetInfo.GetVirtualPath());
            desc.DiffuseVirtualPath = String(Texture::EngineDefault);
        }

        const Material::LoadDesc loadDesc{
            .DiffuseVirtualPath = desc.DiffuseVirtualPath
        };

        json serializedMeta{};
        serializedMeta << assetInfo << loadDesc;

        const fs::path metadataPath{ MakeAssetMetadataPath(EAssetType::Material, assetInfo.GetGuid()) };
        IG_CHECK(!metadataPath.empty());
        if (!SaveJsonToFile(metadataPath, serializedMeta))
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::FailedSaveMetadata>();
        }

        const fs::path assetPath{ MakeAssetPath(EAssetType::Material, assetInfo.GetGuid()) };
        if (!fs::exists(assetPath) && !SaveBlobToFile(assetPath, std::array<uint8_t, 1>{ 0 }))
        {
            return MakeFail<Material::Desc, EMaterialCreateStatus::FailedSaveAsset>();
        }

        return MakeSuccess<Material::Desc, EMaterialCreateStatus>(Material::Desc{ .Info = assetInfo,
                                                                                  .LoadDescriptor = loadDesc });
    }
} // namespace ig