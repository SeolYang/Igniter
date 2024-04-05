#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <Asset/Material.h>
#include <Asset/AssetCache.h>
#include <Asset/Common.h>

namespace ig
{
    class StaticMesh;
    template <>
    constexpr inline EAssetType AssetTypeOf_v<StaticMesh> = EAssetType::StaticMesh;

    struct StaticMeshImportDesc
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

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
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        uint32_t NumVertices{ 0 };
        uint32_t NumIndices{ 0 };
        size_t CompressedVerticesSizeInBytes{ 0 };
        size_t CompressedIndicesSizeInBytes{ 0 };
        Guid MaterialGuid{};
        /* #sy_todo Add AABB Info */
    };

    class GpuBuffer;
    class GpuView;
    class Material;
    class StaticMesh final
    {
    public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc = StaticMeshLoadDesc;
        using Desc = AssetDesc<StaticMesh>;

    public:
        StaticMesh(Desc snapshot, DeferredHandle<GpuBuffer> vertexBuffer,
                   Handle<GpuView, GpuViewManager*> vertexBufferSrv,
                   DeferredHandle<GpuBuffer> indexBuffer,
                   CachedAsset<Material> material);
        StaticMesh(const StaticMesh&) = delete;
        StaticMesh(StaticMesh&&) noexcept = default;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&) = delete;
        StaticMesh& operator=(StaticMesh&&) noexcept = default;

        const Desc& GetSnapshot() const { return snapshot; }
        RefHandle<GpuBuffer> GetVertexBuffer() { return vertexBuffer.MakeRef(); }     // StaticMesh가 생성 되었다면 항상 valid
        RefHandle<GpuView> GetVertexBufferSrv() { return vertexBufferSrv.MakeRef(); } // StaticMesh가 생성 되었다면 항상 valid
        RefHandle<GpuBuffer> GetIndexBuffer() { return indexBuffer.MakeRef(); }       // StaticMesh가 생성 되었다면 항상 valid
        Material& GetMaterial() { return *material; } /* #sy_todo material이 invalidated 되면 engine default로 변환 */

    private:
        Desc snapshot{};
        DeferredHandle<GpuBuffer> vertexBuffer{};
        Handle<GpuView, GpuViewManager*> vertexBufferSrv{};
        DeferredHandle<GpuBuffer> indexBuffer{};
        CachedAsset<Material> material{};
    };
} // namespace ig