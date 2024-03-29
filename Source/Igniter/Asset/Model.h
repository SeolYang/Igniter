#pragma once
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Result.h>
#include <Asset/Common.h>

namespace ig
{
    class GpuBuffer;
    class GpuView;

    struct StaticMeshImportConfig
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

    struct StaticMeshLoadConfig
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

    struct SkeletalMeshImportConfig
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);
    };

    struct SkeletalMeshLoadConfig
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);
    };

    /* Static Mesh */
    struct StaticMesh
    {
    public:
        using ImportConfigType = StaticMeshImportConfig;
        using LoadConfigType = StaticMeshLoadConfig;
        using MetadataType = std::pair<AssetInfo, StaticMeshLoadConfig>;

    public:
        xg::Guid Guid;
        StaticMeshLoadConfig LoadConfig;
        Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> VertexBufferInstance;
        Handle<GpuView, GpuViewManager*> VertexBufferSrv;
        Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> IndexBufferInstance;
        // RefHandle<Material>
    };

    template <>
    constexpr inline EAssetType AssetTypeOf_v<StaticMesh> = EAssetType::StaticMesh;

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
        using Result = Result<StaticMesh::MetadataType, EStaticMeshImportStatus>;
        using Metadata = StaticMesh::MetadataType;

    public:
        static std::vector<Result> ImportStaticMesh(AssetManager& assetManager, const String resPathStr, StaticMesh::ImportConfigType config);
    };

    class StaticMeshLoader
    {
    public:
        static std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
    };
} // namespace ig