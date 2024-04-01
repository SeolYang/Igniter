#pragma once
#include <Core/Handle.h>
#include <Core/Result.h>
#include <Core/String.h>
#include <Asset/Common.h>
#include <Asset/Texture.h>

namespace ig
{
    struct Material;
    template <>
    constexpr inline EAssetType AssetTypeOf_v<Material> = EAssetType::Material;

    struct MaterialCreateDesc final
    {
    public:
        CachedAsset<Texture> Diffuse{};
    };

    struct MaterialLoadDesc final
    {
    public:
        String DiffuseVirtualPath{};
    };

    struct Material final
    {
    public:
        using ImportDesc = void;
        using CreateDesc = MaterialCreateDesc;
        using LoadDesc = MaterialLoadDesc;
        using Desc = AssetDesc<Material>;

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
        static Desc Create(const String virtualPath, MaterialCreateDesc desc);

    private:
        Desc snapshot{};
        CachedAsset<Texture> diffuse{};
    };
} // namespace ig