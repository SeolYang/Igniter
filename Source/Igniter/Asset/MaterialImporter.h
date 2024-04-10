#pragma once
#include <Igniter.h>
#include <Asset/Material.h>

namespace ig
{
    enum class EMaterialCreateStatus
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
        Result<Material::Desc, EMaterialCreateStatus> Import(const AssetInfo& assetInfo, const MaterialCreateDesc& desc);

    private:
        AssetManager& assetManager;
    };
}