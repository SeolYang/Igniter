#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/Material.h"

namespace ig
{
    class StaticMesh;
    template <>
    constexpr inline auto AssetCategoryOf<StaticMesh> = EAssetCategory::StaticMesh;

    struct StaticMeshImportDesc
    {
    public:
        Json&       Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        bool bMakeLeftHanded        = true;
        bool bFlipUVs               = true;
        bool bFlipWindingOrder      = true;
        bool bGenerateNormals       = false;
        bool bSplitLargeMeshes      = false;
        bool bPreTransformVertices  = false;
        bool bImproveCacheLocality  = false;
        bool bGenerateUVCoords      = false;
        bool bGenerateBoundingBoxes = false;

        bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */
    };

    struct StaticMeshLoadDesc
    {
    public:
        Json&       Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        uint32_t NumVertices{0};
        uint32_t NumIndices{0};
        size_t   CompressedVerticesSizeInBytes{0};
        size_t   CompressedIndicesSizeInBytes{0};
        Guid     MaterialGuid{DefaultMaterialGuid};
        /* #sy_todo Add AABB Info */
    };

    class GpuBuffer;
    class GpuView;
    class MaterialAsset;
    class RenderContext;
    class AssetManager;

    class StaticMesh final
    {
    public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc   = StaticMeshLoadDesc;
        using Desc       = AssetDesc<StaticMesh>;

    public:
        StaticMesh(RenderContext& renderContext, AssetManager& assetManager, const Desc& snapshot, const MeshStorage::Handle<VertexSM> vertexSpace, MeshStorage::Handle<U32> vertexIndexSpace, const ManagedAsset<MaterialAsset> material);
        StaticMesh(const StaticMesh&)     = delete;
        StaticMesh(StaticMesh&&) noexcept = default;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&)     = delete;
        StaticMesh& operator=(StaticMesh&&) noexcept = default;

        [[nodiscard]] const Desc&                   GetSnapshot() const { return snapshot; }
        [[nodiscard]] MeshStorage::Handle<VertexSM> GetVertexSpace() const noexcept { return vertexSpace; }
        [[nodiscard]] MeshStorage::Handle<U32>      GetVertexIndexSpace() const noexcept { return vertexIndexSpace; }
        [[nodiscard]] ManagedAsset<MaterialAsset>        GetMaterial() const noexcept { return material; }

    private:
        RenderContext*                renderContext{nullptr};
        AssetManager*                 assetManager{nullptr};
        Desc                          snapshot{ };
        MeshStorage::Handle<VertexSM> vertexSpace{ };
        MeshStorage::Handle<U32>      vertexIndexSpace{ };
        ManagedAsset<MaterialAsset>        material{ };
    };
} // namespace ig
