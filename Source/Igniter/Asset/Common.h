#pragma once
#include <Igniter.h>
#include <Core/String.h>
#include <Core/Handle.h>
#include <Core/GuidBytes.h>

namespace ig::details
{
    inline constexpr std::string_view VirtualPathSeparator = "\\";
    inline constexpr std::string_view MetadataExt = ".metadata";
    inline constexpr std::string_view ResourceRootPath = "Resources";
    inline constexpr std::string_view AssetRootPath = "Assets";
    inline constexpr std::string_view TextureAssetRootPath = "Assets\\Textures";
    inline constexpr std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
    inline constexpr std::string_view SkeletalMeshAssetRootPath = "Assets\\SkeletalMeshes";
    inline constexpr std::string_view AudioAssetRootPath = "Assets\\Audios";
    inline constexpr std::string_view ScriptAssetRootPath = "Assets\\Scripts";
    inline constexpr std::string_view MaterialAssetRootPath = "Assets\\Materials";
} // namespace ig::details

namespace ig
{
    inline constexpr GuidBytes DefaultTextureGuid{ GuidBytesFrom("ec5ba4d0-9b40-40d7-bec6-c4dd41809fd2") };
    inline constexpr GuidBytes DefaultWhiteTextureGuid{ GuidBytesFrom("b4595432-4968-4dd6-b766-13fdcf7435da") };
    inline constexpr GuidBytes DefaultBlackTextureGuid{ GuidBytesFrom("d923e9f9-1651-4393-9636-1a231c7a2b6d") };
    inline constexpr GuidBytes DefaultMaterialGuid{ GuidBytesFrom("ca932248-e2d7-4b4f-9d28-2140f1bf30e3") };
} // namespace ig

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

    enum class EAssetScope
    {
        Managed, /* 수명: 레퍼런스 카운터 > 0 or 중복 Virtual Path 발생 -> Deleted */
        Static,  /* 수명: 에셋 매니저 해제 까지 or 중복 Virtual Path 발생 -> Deleted */
        Engine,  /* 수명: 에셋 매니저 해제 까지, 중복 Virtual Path 발생 -> Ignored */
    };

    /* Common Asset Metadata */
    struct AssetInfo
    {
        friend class AssetManager;

    public:
        AssetInfo() = default;
        AssetInfo(const AssetInfo&) = default;
        AssetInfo(AssetInfo&&) noexcept = default;
        AssetInfo(const String virtualPath, const EAssetType type);
        ~AssetInfo() = default;

        AssetInfo& operator=(const AssetInfo&) = default;
        AssetInfo& operator=(AssetInfo&&) noexcept = default;

        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

        [[nodiscard]] bool IsValid() const;

        Guid GetGuid() const { return guid; }
        String GetVirtualPath() const { return virtualPath; }
        EAssetType GetType() const { return type; }
        EAssetScope GetScope() const { return scope; }
        const std::vector<String>& GetVirtualPathHierarchy() const { return virtualPathHierarchy; }

        void SetVirtualPath(const String newVirtualPath);
        void SetScope(const EAssetScope newScope)
        {
            scope = (newScope == EAssetScope::Engine) ? EAssetScope::Static : newScope;
        }

    private:
        void ConstructVirtualPathHierarchy();
        void MarkAsEngineDefault() { scope = EAssetScope::Engine; }
        void SetGuid(const Guid newGuid) { guid = newGuid; }

    private:
        uint64_t creationTime = 0;
        Guid guid{};
        String virtualPath{};
        std::vector<String> virtualPathHierarchy{};
        EAssetType type = EAssetType::Unknown;
        EAssetScope scope = EAssetScope::Managed;
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

        {
            asset.OnImGui()
        };
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
    fs::path MakeAssetPath(const EAssetType type, const Guid& guid);

    /* Refer to ./Assets/{AssetType}/{GUID}.metadata */
    fs::path MakeAssetMetadataPath(const EAssetType type, const Guid& guid);

    bool HasImportedBefore(const fs::path& resPath);

    bool IsMetadataPath(const fs::path& resPath);

    Guid ConvertMetadataPathToGuid(fs::path path);

    bool IsValidVirtualPath(const String virtualPath);

    String MakeVirtualPathPreferred(const String virtualPath);
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
        return std::format_to(ctx.out(), "{}:{}({})", info.GetType(), info.GetVirtualPath(), info.GetGuid());
    }
};
