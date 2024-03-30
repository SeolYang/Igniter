#pragma once
#include <Igniter.h>
#include <Core/String.h>

namespace ig
{
    enum class EAssetType
    {
        Unknown,
        Texture,
        StaticMesh,
        SkeletalMesh,
        Shader,
        Audio,
        Script,
        // Scene
    };

    struct ResourceInfo
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        EAssetType Type = EAssetType::Unknown;
    };

    /* Common Asset Metadata */
    struct AssetInfo
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

        [[nodiscard]] bool IsValid() const { return Guid.isValid() && VirtualPath.IsValid() && Type != EAssetType::Unknown; }

    public:
        uint64_t CreationTime = 0;
        xg::Guid Guid{};
        String VirtualPath{};
        EAssetType Type = EAssetType::Unknown;
        bool bIsPersistent = false;
    };

    template <typename T>
    constexpr EAssetType AssetTypeOf_v = EAssetType::Unknown;

    template <typename T>
        requires(AssetTypeOf_v<T> != EAssetType::Unknown)
    struct AssetDesc
    {
        AssetInfo Info;
        T::LoadDesc LoadDescriptor;
    };

    template <typename T>
    concept Asset = requires(T asset) {
        typename T::ImportDesc;
        typename T::LoadDesc;
        typename T::Desc;
        {
            asset.GetSnapshot()
        } -> std::same_as<typename T::Desc>;
    } && std::is_same_v<typename T::Desc, AssetDesc<T>> && AssetTypeOf_v<T> != EAssetType::Unknown;

    template <typename T>
    constexpr inline bool IsAsset_v = false;

    template <Asset T>
    constexpr inline bool IsAsset_v<T> = true;

    namespace details
    {
        constexpr inline std::string_view MetadataExt = ".metadata";
        constexpr inline std::string_view ResourceRootPath = "Resources";
        constexpr inline std::string_view AssetRootPath = "Assets";
        constexpr inline std::string_view TextureAssetRootPath = "Assets\\Textures";
        constexpr inline std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
        constexpr inline std::string_view SkeletalMeshAssetRootPath = "Assets\\SkeletalMeshes";
        constexpr inline std::string_view ShaderAssetRootPath = "Assets\\Shaders";
        constexpr inline std::string_view AudioAssetRootPath = "Assets\\Audios";
        constexpr inline std::string_view ScriptAssetRootPath = "Assets\\Scripts";

    } // namespace details
} // namespace ig
