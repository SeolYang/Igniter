#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Core/String.h"
#include "Igniter/Asset/Texture.h"

namespace ig
{
    class MaterialAsset;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<MaterialAsset> = EAssetCategory::Material;

    struct MaterialAssetCreateDesc final
    {
    public:
        String DiffuseVirtualPath{ };
    };

    struct MaterialAssetLoadDesc final
    {
    public:
        Json&       Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        Guid DiffuseTexGuid{DefaultTextureGuid};
    };

    class AssetManager;

    class MaterialAsset final
    {
    public:
        using ImportDesc = MaterialAssetCreateDesc;
        using LoadDesc   = MaterialAssetLoadDesc;
        using Desc       = AssetDesc<MaterialAsset>;

        friend class AssetManager;

    public:
        MaterialAsset(AssetManager& assetManager, const Desc& snapshot, const ManagedAsset<Texture> diffuse);
        MaterialAsset(const MaterialAsset&)     = delete;
        MaterialAsset(MaterialAsset&&) noexcept = default;
        ~MaterialAsset();

        MaterialAsset& operator=(const MaterialAsset&)     = delete;
        MaterialAsset& operator=(MaterialAsset&&) noexcept = default;

        const Desc&           GetSnapshot() const { return snapshot; }
        ManagedAsset<Texture> GetDiffuse() const { return diffuse; }

    public:
        /* #sy_wip Common 헤더로 이동 */
        static constexpr std::string_view EngineDefault{"Engine\\Default"};

    private:
        AssetManager*         assetManager{nullptr};
        Desc                  snapshot{ };
        ManagedAsset<Texture> diffuse{ };
    };
} // namespace ig
