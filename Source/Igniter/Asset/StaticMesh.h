#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <Asset/Common.h>

namespace ig
{
    struct StaticMesh;
    template <>
    constexpr inline EAssetType AssetTypeOf_v<StaticMesh> = EAssetType::StaticMesh;

    class GpuBuffer;
    class GpuView;
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
        /* #sy_todo Add AABB Info */
        // std::vector<xg::Guid> ... or std::vector<std::string> materials; Material?
    };

    struct StaticMesh final
    {
    public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc = StaticMeshLoadDesc;
        using Desc = AssetDesc<StaticMesh>;

    public:
        StaticMesh(Desc snapshot, DeferredHandle<GpuBuffer> vertexBuffer, Handle<GpuView, GpuViewManager*> vertexBufferSrv, DeferredHandle<GpuBuffer> indexBuffer);
        StaticMesh(const StaticMesh&) = delete;
        StaticMesh(StaticMesh&&) noexcept = default;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&) = delete;
        StaticMesh& operator=(StaticMesh&&) noexcept = default;

        Desc GetSnapshot() const { return snapshot; }
        RefHandle<GpuBuffer> GetVertexBuffer() { return vertexBuffer.MakeRef(); }
        RefHandle<GpuView> GetVertexBufferSrv() { return vertexBufferSrv.MakeRef(); }
        RefHandle<GpuBuffer> GetIndexBuffer() { return indexBuffer.MakeRef(); }

    private:
        Desc snapshot{};
        DeferredHandle<GpuBuffer> vertexBuffer{};
        Handle<GpuView, GpuViewManager*> vertexBufferSrv{};
        DeferredHandle<GpuBuffer> indexBuffer{};
    };

    enum class EStaticMeshImportStatus
    {
        Success,
        FileDoesNotExist,
        FailedLoadFromFile,
        FailedSaveMetadataToFile,
        FailedSaveAssetToFile,
        EmptyVertices,
        EmptyIndices,
    };

    class AssetManager;
    class TextureImporter;
    class StaticMeshImporter
    {
    public:
        static std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> ImportStaticMesh(AssetManager& assetManager, const String resPathStr, StaticMesh::ImportDesc desc);
    };

    class StaticMeshLoader
    {
    public:
        static std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
    };
} // namespace ig