#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/GuidBytes.h"

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
    inline constexpr std::string_view MapAssetRootPath = "Assets\\Maps";
} // namespace ig::details

namespace ig
{
    inline constexpr GuidBytes DefaultTextureGuid{GuidBytesFrom("ec5ba4d0-9b40-40d7-bec6-c4dd41809fd2")};
    inline constexpr GuidBytes DefaultWhiteTextureGuid{GuidBytesFrom("b4595432-4968-4dd6-b766-13fdcf7435da")};
    inline constexpr GuidBytes DefaultBlackTextureGuid{GuidBytesFrom("d923e9f9-1651-4393-9636-1a231c7a2b6d")};
    inline constexpr GuidBytes DefaultMaterialGuid{GuidBytesFrom("ca932248-e2d7-4b4f-9d28-2140f1bf30e3")};

    enum class EAssetCategory
    {
        Unknown,
        Texture,
        StaticMesh,
        SkeletalMesh,
        Audio,
        Material,
        Map,
    };
    template <typename T>
    inline constexpr EAssetCategory AssetCategoryOf = EAssetCategory::Unknown;

    template <>
    constexpr inline EAssetCategory AssetCategoryOf<class Material> = EAssetCategory::Material;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<class Map> = EAssetCategory::Map;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<class StaticMesh> = EAssetCategory::StaticMesh;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<class Texture> = EAssetCategory::Texture;

    struct ResourceInfo
    {
      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

      public:
        EAssetCategory Category = EAssetCategory::Unknown;
    };

    enum class EAssetScope
    {
        Managed, /* 수명: 레퍼런스 카운터 > 0*/
        Static,  /* 수명: 에셋 매니저 해제 까지, 에셋 인스턴스 대체 가능 */
        Engine,  /* 수명: 에셋 매니저 해제 까지, 에셋 인스턴스 대체 불가  */
    };

    /* Common Asset Metadata */
    struct AssetInfo
    {
        friend class AssetManager;

      public:
        AssetInfo() = default;
        AssetInfo(const AssetInfo&) = default;
        AssetInfo(AssetInfo&&) noexcept = default;
        AssetInfo(const String virtualPath, const EAssetCategory category);
        ~AssetInfo() = default;

        AssetInfo& operator=(const AssetInfo&) = default;
        AssetInfo& operator=(AssetInfo&&) noexcept = default;

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

        [[nodiscard]] bool IsValid() const;

        const Guid& GetGuid() const { return guid; }
        String GetVirtualPath() const { return virtualPath; }
        EAssetCategory GetCategory() const { return category; }
        EAssetScope GetScope() const { return scope; }
        const Vector<String>& GetVirtualPathHierarchy() const { return virtualPathHierarchy; }

        void SetVirtualPath(const String newVirtualPath);
        void SetScope(const EAssetScope newScope) { scope = (newScope == EAssetScope::Engine) ? EAssetScope::Static : newScope; }

      private:
        AssetInfo(const Guid& guid, const String virtualPath, const EAssetCategory category, const EAssetScope scope);
        void ConstructVirtualPathHierarchy();
        void SetGuid(const Guid& newGuid) { this->guid = newGuid; }

      private:
        uint64_t creationTime = 0;
        Guid guid{};
        String virtualPath{};
        Vector<String> virtualPathHierarchy{};
        EAssetCategory category = EAssetCategory::Unknown;
        EAssetScope scope = EAssetScope::Managed;
    };

    template <typename T>
    struct AssetDesc
    {
        AssetInfo Info;
        T::LoadDesc LoadDescriptor;
    };

    template <typename T>
    using ManagedAsset = Handle<T, class AssetManager>;

    /* Refer to {ResourcePath}.metadata */
    Path MakeResourceMetadataPath(Path resPath);

    Path GetAssetDirectoryPath(const EAssetCategory type);

    // {ResourcePath}/temp
    Path GetTempAssetDirectoryPath(const EAssetCategory type);

    /* Refer to ./Asset/{AssetType}/{GUID} */
    Path MakeAssetPath(const EAssetCategory type, const Guid& guid);

    /* Refer to ./Assets/{AssetType}/{GUID}.metadata */
    Path MakeAssetMetadataPath(const EAssetCategory type, const Guid& guid);

    /* Refer to ./Assets/{AssetType}/temp/{GUID}.metadata */
    Path MakeTempAssetPath(const EAssetCategory type, const Guid& guid);

    /* Refer to ./Assets/{AssetType}/temp/{GUID}.metadata */
    Path MakeTempAssetMetadataPath(const EAssetCategory type, const Guid& guid);

    bool HasImportedBefore(const Path& resPath);

    bool IsMetadataPath(const Path& resPath);

    Guid ConvertMetadataPathToGuid(Path path);

    bool IsValidVirtualPath(const String virtualPath);

    String MakeVirtualPathPreferred(const String virtualPath);
} // namespace ig

template <>
struct std::formatter<ig::AssetInfo>
{
  public:
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FrameContext>
    auto format(const ig::AssetInfo& info, FrameContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}:{}({})", info.GetCategory(), info.GetVirtualPath(), info.GetGuid());
    }
};
