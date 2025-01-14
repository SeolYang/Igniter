#pragma once
#include "Igniter/Asset/Material.h"

namespace ig
{
    enum class EMaterialAssetImportStatus
    {
        Success,
        InvalidAssetInfo,
        InvalidAssetType,
        FailedSaveMetadata,
        FailedSaveAsset
    };

    class AssetManager;

    class MaterialImporter final
    {
        friend class AssetManager;

      public:
        MaterialImporter(AssetManager& assetManager);
        MaterialImporter(const MaterialImporter&) = delete;
        MaterialImporter(MaterialImporter&&) noexcept = delete;
        ~MaterialImporter() = default;

        MaterialImporter& operator=(const MaterialImporter&) = delete;
        MaterialImporter& operator=(MaterialImporter&&) noexcept = delete;

      private:
        Result<Material::Desc, EMaterialAssetImportStatus> Import(const AssetInfo& assetInfo, const MaterialAssetCreateDesc& desc);

      private:
        AssetManager& assetManager;
    };
} // namespace ig
