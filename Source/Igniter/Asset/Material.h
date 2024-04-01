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

    enum class EMaterialCreateStatus
    {
        Success,
        InvalidVirtualPath,
        EmptyVirtualPath,
        FailedSaveMetadata,
        FailedSaveAsset
    };

    struct MaterialCreateDesc final
    {
    public:
        String VirtualPath{};
        CachedAsset<Texture> Diffuse{};
    };

    struct MaterialLoadDesc final
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        String DiffuseVirtualPath{};
    };

    class AssetManager;
    struct Material final
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

    private:
        static Result<Desc, EMaterialCreateStatus> Create(MaterialCreateDesc desc);

    private:
        Desc snapshot{};
        CachedAsset<Texture> diffuse{};
    };
} // namespace ig