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

        struct StaticMeshData
        {
            Vector<VertexSM> Vertices;
            Array<Vector<U32>, StaticMesh::kMaxNumLods> IndicesPerLod;
            AABB BoundingVolume;
            U8 NumLods = 1;
        };

        struct CompressedStaticMeshData
        {
            Vector<U8> CompressedVertices;
            Array<Vector<U8>, StaticMesh::kMaxNumLods> CompressedIndicesPerLod;
        };

      public:
        explicit StaticMeshImporter(AssetManager& assetManager);
        StaticMeshImporter(const StaticMeshImporter&) = delete;
        StaticMeshImporter(StaticMeshImporter&&) noexcept = delete;
        ~StaticMeshImporter() = default;

        StaticMeshImporter& operator=(const StaticMeshImporter&) = delete;
        StaticMeshImporter& operator=(StaticMeshImporter&&) noexcept = delete;

      private:
        Vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> Import(const String resPathStr, const StaticMesh::ImportDesc& desc);

        static U32 MakeAssimpImportFlagsFromDesc(const StaticMesh::ImportDesc& desc);
        static Size ImportMaterialsFromScene(AssetManager& assetManager, const aiScene& scene);
        static StaticMeshData ProcessMeshData(const aiMesh& mesh);
        static void OptimizeMeshData(StaticMeshData& meshData, const StaticMeshImportDesc& desc);
        static void GenerateLODs(StaticMeshData& meshData, const U8 numLods, const F32 maxSimplificationFactor);
        static CompressedStaticMeshData CompressMeshData(const StaticMeshData& meshData);
        static Result<StaticMesh::Desc, EStaticMeshImportStatus> SaveAsAsset(const std::string_view meshName, const StaticMeshData& meshData, const CompressedStaticMeshData& compressedMeshData);

      private:
        AssetManager& assetManager;
    };
} // namespace ig
