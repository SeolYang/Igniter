#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Asset/AssetCache.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/MaterialImporter.h"

IG_DECLARE_LOG_CATEGORY(MaterialImporter);

//IG_DEFINE_LOG_CATEGORY(MaterialImporter);

namespace ig
{
    MaterialImporter::MaterialImporter(AssetManager& assetManager)
        : assetManager(assetManager)
    {}

    Result<Material::Desc, EMaterialAssetImportStatus> MaterialImporter::Import(const AssetInfo& assetInfo, const MaterialAssetCreateDesc& desc)
    {
        if (!assetInfo.IsValid())
        {
            return MakeFail<Material::Desc, EMaterialAssetImportStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::Material)
        {
            return MakeFail<Material::Desc, EMaterialAssetImportStatus::InvalidAssetType>();
        }

        const Handle<Texture> diffuse{assetManager.LoadTexture(desc.DiffuseVirtualPath)};
        Guid diffuseTexGuid{DefaultTextureGuid};
        if (diffuse)
        {
            const Texture* diffusePtr = assetManager.Lookup(diffuse);
            IG_CHECK(diffusePtr != nullptr);
            const Texture::Desc& diffuseDescSnapshot{diffusePtr->GetSnapshot()};
            const AssetInfo& diffuseInfo{diffuseDescSnapshot.Info};
            diffuseTexGuid = diffuseInfo.GetGuid();
        }
        IG_CHECK(diffuseTexGuid.isValid());

        const Material::LoadDesc loadDesc{.DiffuseTexGuid = diffuseTexGuid};

        Json serializedMeta{};
        serializedMeta << assetInfo << loadDesc;

        const Path metadataPath{MakeAssetMetadataPath(EAssetCategory::Material, assetInfo.GetGuid())};
        IG_CHECK(!metadataPath.empty());
        if (!SaveJsonToFile(metadataPath, serializedMeta))
        {
            return MakeFail<Material::Desc, EMaterialAssetImportStatus::FailedSaveMetadata>();
        }

        const Path assetPath{MakeAssetPath(EAssetCategory::Material, assetInfo.GetGuid())};
        if (!fs::exists(assetPath) && !SaveBlobToFile(assetPath, std::array<uint8_t, 1>{0}))
        {
            return MakeFail<Material::Desc, EMaterialAssetImportStatus::FailedSaveAsset>();
        }

        return MakeSuccess<Material::Desc, EMaterialAssetImportStatus>(Material::Desc{.Info = assetInfo, .LoadDescriptor = loadDesc});
    }
} // namespace ig
