#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Core/String.h"
#include "Igniter/Asset/Texture.h"

namespace ig
{
    class Material;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<Material> = EAssetCategory::Material;

    struct MaterialCreateDesc final
    {
    public:
        String DiffuseVirtualPath{};
    };

    struct MaterialLoadDesc final
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        Guid DiffuseTexGuid{ DefaultTextureGuid };
    };

    class AssetManager;
    class Material final
    {
    public:
        using ImportDesc = MaterialCreateDesc;
        using LoadDesc = MaterialLoadDesc;
        using Desc = AssetDesc<Material>;

        friend class AssetManager;

    public:
        Material(AssetManager& assetManager, const Desc& snapshot, const ManagedAsset<Texture> diffuse);
        Material(const Material&) = delete;
        Material(Material&&) noexcept = default;
        ~Material();

        Material& operator=(const Material&) = delete;
        Material& operator=(Material&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        ManagedAsset<Texture> GetDiffuse() const { return diffuse; }

    public:
        /* #sy_wip Common 헤더로 이동 */
        static constexpr std::string_view EngineDefault{ "Engine\\Default" };

    private:
        AssetManager* assetManager{ nullptr };
        Desc snapshot{};
        ManagedAsset<Texture> diffuse{};
    };
}    // namespace ig
