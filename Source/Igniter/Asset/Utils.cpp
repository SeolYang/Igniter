#include <Igniter.h>
#include <Asset/Utils.h>

namespace ig
{
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
            case EAssetType::Shader:
                return fs::path{ details::ShaderAssetRootPath };
            case EAssetType::Audio:
                return fs::path{ details::AudioAssetRootPath };
            case EAssetType::Script:
                return fs::path{ details::ScriptAssetRootPath };
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
                       { return static_cast<char>(::tolower(static_cast<int>(character))); });

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
} // namespace ig