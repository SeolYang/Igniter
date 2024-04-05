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
    constexpr inline EAssetType AssetTypeOf_v<Material> = EAssetType::Material;

    struct MaterialCreateDesc final
    {
    public:
        String DiffuseVirtualPath{};
    };

    struct MaterialLoadDesc final
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        /* #sy_wip Guid로 대체 */
        String DiffuseVirtualPath{};
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
        Material(Desc snapshot, CachedAsset<Texture> diffuse);
        Material(const Material&) = delete;
        Material(Material&&) noexcept = default;
        ~Material();

        Material& operator=(const Material&) = delete;
        Material& operator=(Material&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        Texture& GetDiffuse();

    public:
        static constexpr std::string_view EngineDefault{ "Engine\\Default" };

    private:
        Desc snapshot{};
        CachedAsset<Texture> diffuse{};
    };
} // namespace ig