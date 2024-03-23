#include <Igniter.h>
#include <Asset/Utils.h>

namespace ig
{
    fs::path MakeResourceMetadataPath(fs::path resPath)
    {
        resPath += details::MetadataExt;
        return resPath;
    }

    fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid)
    {
        fs::path newAssetPath{};
        if (type != EAssetType::Unknown && guid.isValid())
        {
            switch (type)
            {
                case EAssetType::Texture:
                    newAssetPath = details::TextureAssetRootPath;
                    break;

                case EAssetType::StaticMesh:
                    newAssetPath = details::StaticMeshAssetRootPath;
                    break;

                case EAssetType::SkeletalMesh:
                    newAssetPath = details::SkeletalMeshAssetRootPath;
                    break;

                case EAssetType::Shader:
                    newAssetPath = details::ShaderAssetRootPath;
                    break;

                case EAssetType::Audio:
                    newAssetPath = details::AudioAssetRootPath;
                    break;

                case EAssetType::Script:
                    newAssetPath = details::ScriptAssetRootPath;
                    break;
            }

            newAssetPath /= guid.str();
        }
        else
        {
            IG_CHECK_NO_ENTRY();
        }

        return newAssetPath;
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

    void details::CreateAssetDirectories()
    {
        if (!fs::exists(TextureAssetRootPath))
        {
            fs::create_directories(TextureAssetRootPath);
        }

        if (!fs::exists(StaticMeshAssetRootPath))
        {
            fs::create_directories(StaticMeshAssetRootPath);
        }

        if (!fs::exists(SkeletalMeshAssetRootPath))
        {
            fs::create_directories(SkeletalMeshAssetRootPath);
        }

        if (!fs::exists(ShaderAssetRootPath))
        {
            fs::create_directories(ShaderAssetRootPath);
        }

        if (!fs::exists(AudioAssetRootPath))
        {
            fs::create_directories(AudioAssetRootPath);
        }

        if (!fs::exists(ScriptAssetRootPath))
        {
            fs::create_directories(ScriptAssetRootPath);
        }
    }
} // namespace ig