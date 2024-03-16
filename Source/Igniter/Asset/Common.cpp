#include <Asset/Common.h>

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
            checkNoEntry();
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
} // namespace ig