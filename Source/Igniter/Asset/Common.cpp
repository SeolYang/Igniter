#include <Igniter.h>
#include <Asset/Common.h>
#include <Core/Json.h>
#include <Core/Regex.h>
#include <Core/Timer.h>

namespace ig
{
    Json& ResourceInfo::Serialize(Json& archive) const
    {
        IG_SERIALIZE_ENUM_JSON_SIMPLE(ResourceInfo, archive, Category);
        return archive;
    }

    const Json& ResourceInfo::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_ENUM_JSON_SIMPLE(ResourceInfo, archive, Category, EAssetCategory::Unknown);
        return archive;
    }

    AssetInfo::AssetInfo(const String virtualPath, const EAssetCategory category)
        : AssetInfo(xg::newGuid(), virtualPath, category, EAssetScope::Managed)
    {
    }

    AssetInfo::AssetInfo(const Guid& guid, const String virtualPath, const EAssetCategory category, const EAssetScope scope)
        : creationTime(Timer::Now()), guid(guid), virtualPath(virtualPath), category(category), scope(scope)
    {
        ConstructVirtualPathHierarchy();
    }

    namespace key
    {
        constexpr inline std::string_view AssetInfo{"AssetInfo"};
        constexpr inline std::string_view CreationTime{"CreationTime"};
        constexpr inline std::string_view Guid{"Guid"};
        constexpr inline std::string_view VirtualPath{"VirtualPath"};
        constexpr inline std::string_view Category{"Category"};
        constexpr inline std::string_view Scope{"Scope"};
    }    // namespace key

    Json& AssetInfo::Serialize(Json& archive) const
    {
        IG_SERIALIZE_JSON(archive, creationTime, key::AssetInfo, key::CreationTime);
        IG_SERIALIZE_GUID_JSON(archive, guid, key::AssetInfo, key::Guid);
        IG_SERIALIZE_JSON(archive, virtualPath, key::AssetInfo, key::VirtualPath);
        IG_SERIALIZE_ENUM_JSON(archive, category, key::AssetInfo, key::Category);
        IG_SERIALIZE_ENUM_JSON(archive, scope, key::AssetInfo, key::Scope);
        return archive;
    }

    const Json& AssetInfo::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_JSON(archive, creationTime, key::AssetInfo, key::CreationTime, 0);
        IG_DESERIALIZE_GUID_JSON(archive, guid, key::AssetInfo, key::Guid, xg::Guid{});
        IG_DESERIALIZE_JSON(archive, virtualPath, key::AssetInfo, key::VirtualPath, String{});
        IG_DESERIALIZE_ENUM_JSON(archive, category, key::AssetInfo, key::Category, EAssetCategory::Unknown);
        IG_DESERIALIZE_ENUM_JSON(archive, scope, key::AssetInfo, key::Scope, EAssetScope::Managed);
        ConstructVirtualPathHierarchy();
        return archive;
    }

    bool AssetInfo::IsValid() const
    {
        return guid.isValid() && IsValidVirtualPath(virtualPath) && category != EAssetCategory::Unknown;
    }

    void AssetInfo::SetVirtualPath(const String newVirtualPath)
    {
        virtualPath = newVirtualPath;
        ConstructVirtualPathHierarchy();
    }

    void AssetInfo::ConstructVirtualPathHierarchy()
    {
        IG_CHECK(IsValidVirtualPath(virtualPath));
        virtualPathHierarchy = virtualPath.Split(details::VirtualPathSeparator);
    }

    Path MakeResourceMetadataPath(Path resPath)
    {
        resPath += details::MetadataExt;
        return resPath;
    }

    Path GetAssetDirectoryPath(const EAssetCategory type)
    {
        IG_CHECK(type != EAssetCategory::Unknown);
        switch (type)
        {
            case EAssetCategory::Texture:
                return Path{details::TextureAssetRootPath};
            case EAssetCategory::StaticMesh:
                return Path{details::StaticMeshAssetRootPath};
            case EAssetCategory::SkeletalMesh:
                return Path{details::SkeletalMeshAssetRootPath};
            case EAssetCategory::Audio:
                return Path{details::AudioAssetRootPath};
            case EAssetCategory::Material:
                return Path{details::MaterialAssetRootPath};
            case EAssetCategory::Map:
                return Path{details::MapAssetRootPath};
            [[unlikely]] default:
                IG_CHECK_NO_ENTRY();
                return Path{};
        }
    }

    Path MakeAssetPath(const EAssetCategory type, const xg::Guid& guid)
    {
        IG_CHECK(guid.isValid() && type != EAssetCategory::Unknown);
        return Path{GetAssetDirectoryPath(type)} / guid.str();
    }

    Path MakeAssetMetadataPath(const EAssetCategory type, const xg::Guid& guid)
    {
        Path newAssetPath{MakeAssetPath(type, guid)};
        if (!newAssetPath.empty())
        {
            newAssetPath.replace_extension(details::MetadataExt);
        }

        return newAssetPath;
    }

    bool HasImportedBefore(const Path& resPath)
    {
        return fs::exists(MakeResourceMetadataPath(resPath));
    }

    bool IsMetadataPath(const Path& resPath)
    {
        std::string extension = resPath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](const char character) { return static_cast<char>(::tolower(static_cast<int>(character))); });

        return extension == details::MetadataExt;
    }

    xg::Guid ConvertMetadataPathToGuid(Path path)
    {
        if (!IsMetadataPath(path))
        {
            return xg::Guid{};
        }

        return xg::Guid{path.replace_extension().filename().string()};
    }

    bool IsValidVirtualPath(const String virtualPath)
    {
        if (!virtualPath.IsValid())
        {
            return false;
        }

        static const std::regex VirtualPathRegex{R"([^\s/\\]*(\\[^\s/\\]*)*)", std::regex_constants::optimize};
        return RegexMatch(virtualPath, VirtualPathRegex);
    }

    String MakeVirtualPathPreferred(const String virtualPath)
    {
        if (!virtualPath.IsValid())
        {
            return String{};
        }

        static const std::regex ReplaceWhiteSpacesRegex{R"(\s+)", std::regex_constants::optimize};
        static const std::regex ReplaceUnpreferredSlash{"/+", std::regex_constants::optimize};
        return RegexReplace(RegexReplace(virtualPath, ReplaceWhiteSpacesRegex, "_"_fs), ReplaceUnpreferredSlash, details::VirtualPathSeparator);
    }
}    // namespace ig
