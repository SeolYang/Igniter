#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/Render/Vertex.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMeshLoader.h"

IG_DEFINE_LOG_CATEGORY(StaticMeshLoader);

namespace ig
{
    StaticMeshLoader::StaticMeshLoader(RenderContext& renderContext, AssetManager& assetManager)
        : renderContext(renderContext), assetManager(assetManager)
    {
    }

    Result<StaticMesh, EStaticMeshLoadStatus> StaticMeshLoader::Load(const StaticMesh::Desc& desc)
    {
        const AssetInfo& assetInfo{desc.Info};
        const StaticMeshLoadDesc& loadDesc{desc.LoadDescriptor};

        if (!assetInfo.IsValid())
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.GetCategory() != EAssetCategory::StaticMesh)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::AssetTypeMismatch>();
        }

        if (loadDesc.NumVertices == 0 || loadDesc.NumIndices == 0 || loadDesc.CompressedVerticesSizeInBytes == 0 ||
            loadDesc.CompressedIndicesSizeInBytes == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidArguments>();
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());
        if (!fs::exists(assetPath))
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FileDoesNotExists>();
        }

        std::vector<uint8_t> blob = LoadBlobFromFile(assetPath);
        if (blob.empty())
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::EmptyBlob>();
        }

        const size_t expectedBlobSize = loadDesc.CompressedVerticesSizeInBytes + loadDesc.CompressedIndicesSizeInBytes;
        if (blob.size() != expectedBlobSize)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::BlobSizeMismatch>();
        }

        GpuBufferDesc vertexBufferDesc{};
        vertexBufferDesc.AsStructuredBuffer<StaticMeshVertex>(loadDesc.NumVertices);
        vertexBufferDesc.DebugName = String(std::format("{}({})_Vertices", assetInfo.GetVirtualPath(), assetInfo.GetGuid()));
        if (vertexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedVerticesSizeInBytes)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidCompressedVerticesSize>();
        }

        GpuBufferDesc indexBufferDesc{};
        indexBufferDesc.AsIndexBuffer<uint32_t>(loadDesc.NumIndices);
        indexBufferDesc.DebugName = String(std::format("{}({})_Indices", assetInfo.GetVirtualPath(), assetInfo.GetGuid()));
        if (indexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedIndicesSizeInBytes)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidCompressedIndicesSize>();
        }

        const RenderResource<GpuBuffer> vertexBuffer = renderContext.CreateBuffer(vertexBufferDesc);
        if (!vertexBuffer)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexBuffer>();
        }

        const RenderResource<GpuView> vertexBufferSrv = renderContext.CreateShaderResourceView(vertexBuffer);
        if (!vertexBufferSrv)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexBufferSrv>();
        }

        const RenderResource<GpuBuffer> indexBuffer = renderContext.CreateBuffer(indexBufferDesc);
        if (!indexBuffer)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateIndexBuffer>();
        }

        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};
        bool bVertexBufferDecodeSucceed = false;
        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadDesc.NumVertices,
                sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                GpuBuffer* vertexBufferPtr = renderContext.Lookup(vertexBuffer);
                IG_CHECK(vertexBufferPtr != nullptr);
                verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBufferPtr);
                bVertexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSyncPoint> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        bool bIndexBufferDecodeSucceed = false;
        UploadContext indicesUploadCtx = gpuUploader.Reserve(indexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedIndicesOffset = loadDesc.CompressedVerticesSizeInBytes;
            const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                loadDesc.NumIndices, blob.data() + compressedIndicesOffset, loadDesc.CompressedIndicesSizeInBytes);

            if (decodeResult == 0)
            {
                GpuBuffer* indexBufferPtr = renderContext.Lookup(indexBuffer);
                IG_CHECK(indexBufferPtr != nullptr);
                indicesUploadCtx.CopyBuffer(0, indexBufferDesc.GetSizeAsBytes(), *indexBufferPtr);
                bIndexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSyncPoint> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

        ManagedAsset<Material> material{assetManager.Load<Material>(loadDesc.MaterialGuid)};
        IG_CHECK(material);

        verticesUploadSync->WaitOnCpu();
        indicesUploadSync->WaitOnCpu();

        if (!bVertexBufferDecodeSucceed)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedDecodeVertexBuffer>();
        }

        if (!bIndexBufferDecodeSucceed)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedDecodeIndexBuffer>();
        }

        return MakeSuccess<StaticMesh, EStaticMeshLoadStatus>(
            StaticMesh{renderContext, assetManager, desc, vertexBuffer, vertexBufferSrv, indexBuffer, material});
    }
}    // namespace ig
