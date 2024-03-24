#pragma once
#include <Asset/Common.h>

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
} // namespace ig