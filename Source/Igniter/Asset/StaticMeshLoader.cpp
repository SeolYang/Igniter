#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Timer.h>
#include <Filesystem/Utils.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/Vertex.h>
#include <Render/GpuViewManager.h>
#include <Render/GpuUploader.h>
#include <Asset/StaticMeshLoader.h>

IG_DEFINE_LOG_CATEGORY(StaticMeshLoader);

namespace ig
{
    StaticMeshLoader::StaticMeshLoader(HandleManager& handleManager,
                                       RenderDevice& renderDevice,
                                       GpuUploader& gpuUploader,
                                       GpuViewManager& gpuViewManager) : handleManager(handleManager),
                                                                         renderDevice(renderDevice),
                                                                         gpuUploader(gpuUploader),
                                                                         gpuViewManager(gpuViewManager)
    {
    }

    Result<StaticMesh, EStaticMeshLoadStatus> StaticMeshLoader::Load(const StaticMesh::Desc& desc)
    {
        const AssetInfo& assetInfo{ desc.Info };
        const StaticMeshLoadDesc& loadDesc{ desc.LoadDescriptor };

        if (!assetInfo.IsValid())
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidAssetInfo>();
        }

        if (assetInfo.Type != EAssetType::StaticMesh)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::AssetTypeMismatch>();
        }

        if (loadDesc.NumVertices == 0 ||
            loadDesc.NumIndices == 0 ||
            loadDesc.CompressedVerticesSizeInBytes == 0 ||
            loadDesc.CompressedIndicesSizeInBytes == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidArguments>();
        }

        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, assetInfo.Guid);
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
        vertexBufferDesc.DebugName = String(std::format("{}({})_Vertices", assetInfo.VirtualPath, assetInfo.Guid));
        if (vertexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedVerticesSizeInBytes)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidCompressedVerticesSize>();
        }

        GpuBufferDesc indexBufferDesc{};
        indexBufferDesc.AsIndexBuffer<uint32_t>(loadDesc.NumIndices);
        indexBufferDesc.DebugName = String(std::format("{}({})_Indices", assetInfo.VirtualPath, assetInfo.Guid));
        if (indexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedIndicesSizeInBytes)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidCompressedIndicesSize>();
        }

        std::optional<GpuBuffer> vertexBuffer = renderDevice.CreateBuffer(vertexBufferDesc);
        if (!vertexBuffer)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexBuffer>();
        }

        auto vertexBufferSrv = gpuViewManager.RequestShaderResourceView(*vertexBuffer);
        if (!vertexBufferSrv)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexBufferSrv>();
        }

        std::optional<GpuBuffer> indexBuffer = renderDevice.CreateBuffer(indexBufferDesc);
        if (!indexBuffer)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateIndexBuffer>();
        }

        bool bVertexBufferDecodeSucceed = false;
        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadDesc.NumVertices,
                                                                sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, 
                                                                loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBuffer);
                bVertexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        bool bIndexBufferDecodeSucceed = false;
        UploadContext indicesUploadCtx = gpuUploader.Reserve(indexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedIndicesOffset = loadDesc.CompressedVerticesSizeInBytes;
            const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(
                reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                loadDesc.NumIndices,
                blob.data() + compressedIndicesOffset, loadDesc.CompressedIndicesSizeInBytes);

            if (decodeResult == 0)
            {
                indicesUploadCtx.CopyBuffer(0, indexBufferDesc.GetSizeAsBytes(), *indexBuffer);
                bIndexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSync> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

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

        return MakeSuccess<StaticMesh, EStaticMeshLoadStatus>(StaticMesh{
            desc,
            { handleManager, std::move(*vertexBuffer) },
            std::move(vertexBufferSrv),
            { handleManager, std::move(*indexBuffer) } });
    }
} // namespace ig