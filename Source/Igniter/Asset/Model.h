#pragma once
#include <PCH.h>
#include <Asset/Common.h>
#include <Core/Handle.h>

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

        /*
         * If enabled, Ignore 'bSplitLargeMeshes' and force enable 'PreTransformVertices'.
         * It cause of loss material details.
         */
        bool bMergeMeshes = false;

        bool bImportTextures = false;  /* Only if textures does not imported before. */
        bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */
    };

    struct StaticMeshLoadConfig
    {
    public:
        json& Serialize(json& archive) const;
        const json& Deserialize(const json& archive);

    public:
        std::string Name{};
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

    class TextureImporter;
    class ModelImporter
    {
    public:
        static std::vector<xg::Guid> ImportAsStatic(TextureImporter& textureImporter, const String resPathStr, std::optional<StaticMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
        /* #sy_todo Impl import as Skeletal Meshes */
        static std::vector<xg::Guid> ImportAsSkeletal(const String resPathStr, std::optional<SkeletalMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
    };

    /* Static Mesh */
    struct StaticMesh
    {
    public:
        StaticMeshLoadConfig LoadConfig;
        Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> VertexBufferInstance;
        Handle<GpuView, GpuViewManager*> VertexBufferSrv;
        Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> IndexBufferInstance;
        // RefHandle<Material>
    };

    class StaticMeshLoader
    {
    public:
        static std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
    };
} // namespace ig