#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/Mesh.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/Material.h"

namespace ig
{
    class StaticMesh;

    struct StaticMeshImportDesc
    {
    public:
        constexpr static U8 kMaxNumLods = 8;

    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        bool bMakeLeftHanded = true;
        bool bFlipUVs = true;
        bool bFlipWindingOrder = true;
        bool bSplitLargeMeshes = false;
        bool bPreTransformVertices = false;
        bool bGenerateUVCoords = false;
        bool bGenerateBoundingBoxes = false;
        bool bImproveCacheLocality = false;
        bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */

        bool bGenerateLODs = true;
    };

    /*
     * Static Mesh Binary Layout
     * Begin->
     * CompressedVertices => [0, CompressedVerticesSize)
     * LOD0.CompressedMeshletVertexIndices => [CompressedVerticesSize, CompressedVerticesSize + LOD0.CompressedMeshletVertexIndicesSize)
     * LOD0.Triangles => [PrevLast, PrevLast+sizeof(U32)*LOD0.NumMeshletTriangles)
     * LOD0.Meshlets => [PrevLast, PrevLast+sizeof(Meshlet)*LOD0.NumMeshlets)
     * ...
     * LOD(N-1).Meshlets => [PrevLast, PrevLast+sizeof(Meshlet)*LOD(N-1).NumMeshlets)
     * <-End
     * assert(N-1 == Mesh.NumLevelOfDetails)
     */
    struct StaticMeshLoadDesc
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        U32 NumVertices{0};
        Bytes CompressedVerticesSize{0};
        U8 NumLevelOfDetails = 1; // assert(>=1)
        Array<U32, Mesh::kMaxMeshLevelOfDetails> NumMeshletVertexIndices{0};
        Array<U32, Mesh::kMaxMeshLevelOfDetails> NumMeshletTriangles{0};
        Array<U32, Mesh::kMaxMeshLevelOfDetails> NumMeshlets{0};
        AABB BoundingBox;

        bool bOverrideLodScreenCoverageThresholds = false;
        Array<F32, Mesh::kMaxMeshLevelOfDetails> LodScreenCoverageThresholds{0.f,};
    };

    class GpuBuffer;
    class GpuView;
    class Material;
    class RenderContext;
    class AssetManager;

    class StaticMesh final
    {
    public:
        constexpr static U8 kMaxNumLods = StaticMeshImportDesc::kMaxNumLods;

    public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc = StaticMeshLoadDesc;
        using Desc = AssetDesc<StaticMesh>;

    public:
        /* New API! */
        StaticMesh(
            RenderContext& renderContext, AssetManager& assetManager,
            const Desc& snapshot,
            const Mesh& newMesh);

        StaticMesh(const StaticMesh&) = delete;
        StaticMesh(StaticMesh&& other) noexcept;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&) = delete;
        StaticMesh& operator=(StaticMesh&& rhs) noexcept;

        [[nodiscard]] const Desc& GetSnapshot() const noexcept { return snapshot; }
        [[nodiscard]] const Mesh& GetMesh() const noexcept { return mesh; }

    private:
        void Destroy();

    private:
        RenderContext* renderContext{nullptr};
        AssetManager* assetManager{nullptr};
        Desc snapshot{};

        Mesh mesh;
    };
} // namespace ig
