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

    template <typename T>
    struct AssetTypeOf
    {
    public:
        constexpr static EAssetType Val = EAssetType::Unknown;
    };

    struct Texture;
    template <>
    struct AssetTypeOf<Texture>
    {
    public:
        constexpr static EAssetType Val = EAssetType::Texture;
    };

    struct StaticMesh;
    template <>
    struct AssetTypeOf<StaticMesh>
    {
    public:
        constexpr static EAssetType Val = EAssetType::StaticMesh;
    };

    template <typename T>
    constexpr EAssetType AssetTypeOf_v = AssetTypeOf<T>::Val;

    template <typename T>
    concept Asset = AssetTypeOf_v<T> != EAssetType::Unknown;

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
