#pragma once
#include "Igniter/Asset/StaticMesh.h"

namespace ig
{
    enum class EStaticMeshImportStatus : U8
    {
        Success,
        FileDoesNotExists,
        FailedLoadFromFile,
        FailedSaveMetadataToFile,
        FailedSaveAssetToFile,
        EmptyVertices,
        EmptyIndices,
    };

    class AssetManager;

    class StaticMeshImporter final
    {
        friend class AssetManager;

      public:
        explicit StaticMeshImporter(AssetManager& assetManager);
        StaticMeshImporter(const StaticMeshImporter&) = delete;
        StaticMeshImporter(StaticMeshImporter&&) noexcept = delete;
        ~StaticMeshImporter() = default;

        StaticMeshImporter& operator=(const StaticMeshImporter&) = delete;
        StaticMeshImporter& operator=(StaticMeshImporter&&) noexcept = delete;

      private:
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> Import(const String resPathStr, const StaticMesh::ImportDesc& desc);

      private:
        AssetManager& assetManager;
    };
} // namespace ig
