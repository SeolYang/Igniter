#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Handle.h>

namespace ig::details
{
    constexpr inline std::string_view MetadataExt = ".metadata";
    constexpr inline std::string_view ResourceRootPath = "Resources";
    constexpr inline std::string_view AssetRootPath = "Assets";
    constexpr inline std::string_view TextureAssetRootPath = "Assets\\Textures";
    constexpr inline std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
    constexpr inline std::string_view SkeletalMeshAssetRootPath = "Assets\\SkeletalMeshes";
    constexpr inline std::string_view AudioAssetRootPath = "Assets\\Audios";
    constexpr inline std::string_view ScriptAssetRootPath = "Assets\\Scripts";
    constexpr inline std::string_view MaterialAssetRootPath = "Assets\\Materials";
} // namespace ig::details

namespace ig
{
    enum class EAssetType
    {
        Unknown,
        Texture,
        StaticMesh,
        SkeletalMesh,
        Audio,
        Material,
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
        bool bIsEngineDefault = false;
    };

    template <typename T>
    inline constexpr EAssetType AssetTypeOf_v = EAssetType::Unknown;

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
        } -> std::same_as<const typename T::Desc&>;
    } && std::is_same_v<typename T::Desc, AssetDesc<T>> && AssetTypeOf_v<T> != EAssetType::Unknown;

    template <typename T>
    inline constexpr bool IsAsset_v = false;

    template <Asset T>
    inline constexpr bool IsAsset_v<T> = true;

    namespace details
    {
        template <Asset T>
        class AssetCache;
    }

    template <Asset T>
    using CachedAsset = UniqueRefHandle<T, details::AssetCache<T>*>;
} // namespace ig

namespace ig
{
    /* Refer to {ResourcePath}.metadata */
    fs::path MakeResourceMetadataPath(fs::path resPath);

    fs::path GetAssetDirectoryPath(const EAssetType type);

    /* Refer to ./Asset/{AssetType}/{GUID} */
    fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid);

    /* Refer to ./Assets/{AssetType}/{GUID}.metadata */
    fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid);

    bool HasImportedBefore(const fs::path& resPath);

    bool IsMetadataPath(const fs::path& resPath);

    xg::Guid ConvertMetadataPathToGuid(fs::path path);

    bool IsValidVirtualPath(const String virtualPath);
} // namespace ig

template <>
struct std::formatter<ig::AssetInfo>
{
public:
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FrameContext>
    auto format(const ig::AssetInfo& info, FrameContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}:{}({})", info.Type, info.VirtualPath, info.Guid);
    }
};
