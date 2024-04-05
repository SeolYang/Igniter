#pragma once
#include <Igniter.h>
#include <Asset/StaticMesh.h>

namespace ig
{
    enum class EStaticMeshLoadStatus
    {
        Success,
        InvalidAssetInfo,
        AssetTypeMismatch,
        InvalidArguments,
        FileDoesNotExists,
        EmptyBlob,
        BlobSizeMismatch,
        InvalidCompressedVerticesSize,
        InvalidCompressedIndicesSize,
        FailedCreateVertexBuffer,
        FailedCreateVertexBufferSrv,
        FailedCreateIndexBuffer,
        FailedDecodeVertexBuffer,
        FailedDecodeIndexBuffer,
    };

    class AssetManager;
    class StaticMeshLoader final
    {
        friend class AssetManager;

    public:
        StaticMeshLoader(HandleManager& handleManager,
                         RenderDevice& renderDevice,
                         GpuUploader& gpuUploader,
                         GpuViewManager& gpuViewManager,
                         AssetManager& assetManager);

        StaticMeshLoader(const StaticMeshLoader&) = delete;
        StaticMeshLoader(StaticMeshLoader&&) noexcept = delete;
        ~StaticMeshLoader() = default;

        StaticMeshLoader& operator=(const StaticMeshLoader&) = delete;
        StaticMeshLoader& operator=(StaticMeshLoader&&) noexcept = delete;

    private:
        Result<StaticMesh, EStaticMeshLoadStatus> Load(const StaticMesh::Desc& desc);

    public:
        HandleManager& handleManager;
        RenderDevice& renderDevice;
        GpuUploader& gpuUploader;
        GpuViewManager& gpuViewManager;
        AssetManager& assetManager;
    };
} // namespace ig