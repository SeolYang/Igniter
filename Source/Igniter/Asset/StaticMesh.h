#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <Render/RenderCommon.h>
#include <Asset/Material.h>
#include <Asset/AssetCache.h>
#include <Asset/Common.h>

namespace ig
{
    class StaticMesh;
    template <>
    constexpr inline EAssetCategory AssetCategoryOf<StaticMesh> = EAssetCategory::StaticMesh;

    struct StaticMeshImportDesc
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        bool bMakeLeftHanded = true;
        bool bFlipUVs = true;
        bool bFlipWindingOrder = true;
        bool bGenerateNormals = false;
        bool bSplitLargeMeshes = false;
        bool bPreTransformVertices = false;
        bool bImproveCacheLocality = false;
        bool bGenerateUVCoords = false;
        bool bGenerateBoundingBoxes = false;

        bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */
    };

    struct StaticMeshLoadDesc
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        uint32_t NumVertices{0};
        uint32_t NumIndices{0};
        size_t CompressedVerticesSizeInBytes{0};
        size_t CompressedIndicesSizeInBytes{0};
        Guid MaterialGuid{DefaultMaterialGuid};
        /* #sy_todo Add AABB Info */
    };

    class GpuBuffer;
    class GpuView;
    class Material;
    class RenderContext;
    class AssetManager;
    class StaticMesh final
    {
    public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc = StaticMeshLoadDesc;
        using Desc = AssetDesc<StaticMesh>;

    public:
        StaticMesh(RenderContext& renderContext, AssetManager& assetManager, const Desc& snapshot, const RenderResource<GpuBuffer> vertexBuffer,
            const RenderResource<GpuView> vertexBufferSrv, const RenderResource<GpuBuffer> indexBuffer, const ManagedAsset<Material> material);
        StaticMesh(const StaticMesh&) = delete;
        StaticMesh(StaticMesh&&) noexcept = default;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&) = delete;
        StaticMesh& operator=(StaticMesh&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        RenderResource<GpuBuffer> GetVertexBuffer() const { return vertexBuffer; }
        RenderResource<GpuView> GetVertexBufferSrv() const { return vertexBufferSrv; }
        RenderResource<GpuBuffer> GetIndexBuffer() const { return indexBuffer; }
        ManagedAsset<Material> GetMaterial() const { return material; }

    private:
        RenderContext* renderContext{nullptr};
        AssetManager* assetManager{nullptr};
        Desc snapshot{};
        RenderResource<GpuBuffer> vertexBuffer{};
        RenderResource<GpuView> vertexBufferSrv{};
        RenderResource<GpuBuffer> indexBuffer{};
        ManagedAsset<Material> material{};
    };
}    // namespace ig
