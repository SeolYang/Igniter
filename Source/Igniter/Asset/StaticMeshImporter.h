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

        struct MeshLod
        {
            Vector<U32> Indices;
            Vector<U32> MeshletVertexIndices;
            Vector<U32> MeshletTriangles;
            Vector<Meshlet> Meshlets;
        };

        struct MeshData
        {
            Vector<VertexSM> Vertices;
            Vector<U8> CompressedVertices;
            Array<MeshLod, Mesh::kMaxMeshLevelOfDetails> LevelOfDetails;
            U8 NumLevelOfDetails = 1; // assert (>=1); LOD 생성을 concurrent 하게 한다 치면 atomic으로?
            AABB BoundingBox;
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

        /* Load LOD0 (+ Remap Vertices & Indices) */
        static void ProcessMeshLod0(const aiMesh& mesh, MeshData& meshData);
        /* Generate LOD 1~LOD (MaxLOD-1): 최대한 많은 LOD 생성 */
        static void GenerateLevelOfDetails(MeshData& meshData);
        /* 각 LOD별로 Meshlet 데이터(Meshlet, Triangles, MeshletVertexIndices) 생성 */
        static void BuildMeshlets(MeshData& meshData);
        /* Mesh의 Vertices 압축 */
        static void CompressMeshVertices(MeshData& meshData);

        static Result<StaticMesh::Desc, EStaticMeshImportStatus> ExportToFile(const std::string_view meshName, const MeshData& meshData);

    private:
        AssetManager& assetManager;
    };
} // namespace ig
