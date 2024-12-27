#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Asset/StaticMesh.h"

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
        FailedCreateVertexSpace,
        FailedCreateVertexIndexSpace,
        FailedDecodeVertexBuffer,
        FailedDecodeIndexBuffer,
    };

    class AssetManager;
    class RenderContext;

    class StaticMeshLoader final
    {
        friend class AssetManager;

    public:
        StaticMeshLoader(RenderContext& renderContext, AssetManager& assetManager);

        StaticMeshLoader(const StaticMeshLoader&)     = delete;
        StaticMeshLoader(StaticMeshLoader&&) noexcept = delete;
        ~StaticMeshLoader()                           = default;

        StaticMeshLoader& operator=(const StaticMeshLoader&)     = delete;
        StaticMeshLoader& operator=(StaticMeshLoader&&) noexcept = delete;

    private:
        Result<StaticMesh, EStaticMeshLoadStatus> Load(const StaticMesh::Desc& desc) const;

    private:
        RenderContext& renderContext;
        AssetManager&  assetManager;
    };
} // namespace ig
