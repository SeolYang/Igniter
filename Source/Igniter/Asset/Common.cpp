#include <Igniter.h>
#include <Asset/Common.h>
#include <Core/Json.h>
#include <Core/Regex.h>

namespace ig
{
    json& ResourceInfo::Serialize(json& archive) const
    {
        const ResourceInfo& info = *this;
        IG_SERIALIZE_ENUM_JSON(ResourceInfo, archive, info, Type);
        return archive;
    }

    const json& ResourceInfo::Deserialize(const json& archive)
    {
        ResourceInfo& info = *this;
        IG_DESERIALIZE_ENUM_JSON(ResourceInfo, archive, info, Type, EAssetType::Unknown);
        return archive;
    }

    json& AssetInfo::Serialize(json& archive) const
    {
        const AssetInfo& info = *this;
        IG_SERIALIZE_JSON(AssetInfo, archive, info, CreationTime);
        IG_SERIALIZE_GUID_JSON(AssetInfo, archive, info, Guid);
        IG_SERIALIZE_JSON(AssetInfo, archive, info, VirtualPath);
        IG_SERIALIZE_ENUM_JSON(AssetInfo, archive, info, Type);
        IG_SERIALIZE_JSON(AssetInfo, archive, info, bIsPersistent);
        return archive;
    }

    const json& AssetInfo::Deserialize(const json& archive)
    {
        AssetInfo& info = *this;
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, CreationTime, 0);
        IG_DESERIALIZE_GUID_JSON(AssetInfo, archive, info, Guid, xg::Guid{});
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, VirtualPath, String{});
        IG_DESERIALIZE_ENUM_JSON(AssetInfo, archive, info, Type, EAssetType::Unknown);
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, bIsPersistent, false);
        return archive;
    }

    fs::path MakeResourceMetadataPath(fs::path resPath)
    {
        resPath += details::MetadataExt;
        return resPath;
    }

    fs::path GetAssetDirectoryPath(const EAssetType type)
    {
        IG_CHECK(type != EAssetType::Unknown);
        switch (type)
        {
            case EAssetType::Texture:
                return fs::path{ details::TextureAssetRootPath };
            case EAssetType::StaticMesh:
                return fs::path{ details::StaticMeshAssetRootPath };
            case EAssetType::SkeletalMesh:
                return fs::path{ details::SkeletalMeshAssetRootPath };
            case EAssetType::Audio:
                return fs::path{ details::AudioAssetRootPath };
            case EAssetType::Material:
                return fs::path{ details::MaterialAssetRootPath };
            [[unlikely]] default:
                IG_CHECK_NO_ENTRY();
                return fs::path{};
        }
    }

    fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid)
    {
        IG_CHECK(guid.isValid() && type != EAssetType::Unknown);
        return fs::path{ GetAssetDirectoryPath(type) } / guid.str();
    }

    fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid)
    {
        fs::path newAssetPath{ MakeAssetPath(type, guid) };
        if (!newAssetPath.empty())
        {
            newAssetPath.replace_extension(details::MetadataExt);
        }

        return newAssetPath;
    }

    bool HasImportedBefore(const fs::path& resPath)
    {
        return fs::exists(MakeResourceMetadataPath(resPath));
    }

    bool IsMetadataPath(const fs::path& resPath)
    {
        std::string extension = resPath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](const char character)
                       {
                           return static_cast<char>(::tolower(static_cast<int>(character)));
                       });

        return extension == details::MetadataExt;
    }

    xg::Guid ConvertMetadataPathToGuid(fs::path path)
    {
        if (!IsMetadataPath(path))
        {
            return xg::Guid{};
        }

        return xg::Guid{ path.replace_extension().filename().string() };
    }

    static const std::regex& GetVirtualPathRegex()
    {
        static const std::regex VirtualPathRegex{ "([a-zA-Z0-9_-])+([\\\\][a-zA-Z0-9_-]+)*", std::regex_constants::optimize };
        return VirtualPathRegex;
    }

    bool IsValidVirtualPath(const String virtualPath)
    {
        if (!virtualPath.IsValid())
        {
            return false;
        }
        return RegexMatch(virtualPath, GetVirtualPathRegex());
    }
} // namespace ig