#pragma once
#include <Core/Handle.h>
#include <Core/Result.h>
#include <Core/String.h>
#include <Asset/Common.h>
#include <Asset/Texture.h>

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
        Guid DiffuseTexGuid{DefaultTextureGuid};
    };

    class AssetManager;

    class Material final
    {
    public:
        using ImportDesc = void;
        using LoadDesc = MaterialLoadDesc;
        using Desc = AssetDesc<Material>;

        friend class AssetManager;

    public:
        Material(const Desc& snapshot, CachedAsset<Texture> diffuse);
        Material(const Material&) = delete;
        Material(Material&&) noexcept = default;
        ~Material();

        Material& operator=(const Material&) = delete;
        Material& operator=(Material&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        RefHandle<Texture> GetDiffuse() { return diffuse.MakeRef(); }

    public:
        /* #sy_wip Common 헤더로 이동 */
        static constexpr std::string_view EngineDefault{"Engine\\Default"};

    private:
        Desc snapshot{};
        CachedAsset<Texture> diffuse{};
    };
}    // namespace ig
