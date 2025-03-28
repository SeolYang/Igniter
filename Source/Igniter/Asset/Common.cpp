#include "Igniter/Igniter.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Regex.h"
#include "Igniter/Core/Timer.h"

namespace ig
{
    Json& ResourceInfo::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(ResourceInfo, archive, Category);
        return archive;
    }

    const Json& ResourceInfo::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_FROM_JSON_FALLBACK(ResourceInfo, archive, Category, EAssetCategory::Unknown);
        return archive;
    }

    AssetInfo::AssetInfo(const std::string_view virtualPath, const EAssetCategory category)
        : AssetInfo(xg::newGuid(), virtualPath, category, EAssetScope::Managed)
    {}

    AssetInfo::AssetInfo(const Guid& guid, const std::string_view virtualPath, const EAssetCategory category, const EAssetScope scope)
        : creationTime(Timer::Now())
        , guid(guid)
        , virtualPath(virtualPath)
        , category(category)
        , scope(scope)
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
    } // namespace key

    Json& AssetInfo::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(AssetInfo, archive, creationTime);
        IG_SERIALIZE_TO_JSON(AssetInfo, archive, guid);
        IG_SERIALIZE_TO_JSON(AssetInfo, archive, virtualPath);
        IG_SERIALIZE_TO_JSON(AssetInfo, archive, category);
        IG_SERIALIZE_TO_JSON(AssetInfo, archive, scope);
        return archive;
    }

    const Json& AssetInfo::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(AssetInfo, archive, creationTime);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(AssetInfo, archive, guid);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(AssetInfo, archive, virtualPath);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(AssetInfo, archive, category);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(AssetInfo, archive, scope);
        ConstructVirtualPathHierarchy();
        return archive;
    }

    bool AssetInfo::IsValid() const
    {
        return guid.isValid() && IsValidVirtualPath(virtualPath) && category != EAssetCategory::Unknown;
    }

    void AssetInfo::SetVirtualPath(const std::string_view newVirtualPath)
    {
        virtualPath = newVirtualPath;
        ConstructVirtualPathHierarchy();
    }

    void AssetInfo::ConstructVirtualPathHierarchy()
    {
        IG_CHECK(IsValidVirtualPath(virtualPath));
        virtualPathHierarchy = Split(virtualPath, details::VirtualPathSeparator);
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

    Path GetTempAssetDirectoryPath(const EAssetCategory type)
    {
        return GetAssetDirectoryPath(type) / "temp";
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

    Path MakeTempAssetPath(const EAssetCategory type, const Guid& guid)
    {
        IG_CHECK(guid.isValid() && type != EAssetCategory::Unknown);
        return Path{GetAssetDirectoryPath(type)} / "temp" / guid.str();
    }

    Path MakeTempAssetMetadataPath(const EAssetCategory type, const Guid& guid)
    {
        Path newAssetPath{MakeTempAssetPath(type, guid)};
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
            [](const char character)
            {
                return static_cast<char>(::tolower(static_cast<int>(character)));
            });

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

    bool IsValidVirtualPath(const std::string_view virtualPath)
    {
        if (virtualPath.empty())
        {
            return false;
        }

        static const std::regex VirtualPathRegex{R"([^\s/\\]*(\\[^\s/\\]*)*)", std::regex_constants::optimize};
        return RegexMatch(virtualPath, VirtualPathRegex);
    }

    std::string MakeVirtualPathPreferred(const std::string_view virtualPath)
    {
        if (virtualPath.empty())
        {
            return {};
        }

        static const std::regex ReplaceWhiteSpacesRegex{R"(\s+)", std::regex_constants::optimize};
        static const std::regex ReplaceUnpreferredSlash{"/+", std::regex_constants::optimize};
        return RegexReplace(RegexReplace(virtualPath, ReplaceWhiteSpacesRegex, "_"), ReplaceUnpreferredSlash, details::VirtualPathSeparator);
    }
} // namespace ig
