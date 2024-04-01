#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Timer.h>
#include <Core/Serializable.h>
#include <Filesystem/Utils.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/Vertex.h>
#include <Render/GpuViewManager.h>
#include <Render/GpuUploader.h>
#include <Asset/Utils.h>
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

    std::optional<StaticMesh> StaticMeshLoader::_Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
    {
        IG_LOG(StaticMeshLoader, Info, "Load static mesh {}.", guid.str());
        TempTimer tempTimer;
        tempTimer.Begin();

        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, guid);
        const fs::path assetMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, guid);
        if (!fs::exists(assetPath))
        {
            IG_LOG(StaticMeshLoader, Error, "Static Mesh Asset \"{}\" does not exists.", assetPath.string());
            return std::nullopt;
        }

        if (!fs::exists(assetMetaPath))
        {
            IG_LOG(StaticMeshLoader, Error, "Static Mesh Asset metadata \"{}\" does not exists.", assetMetaPath.string());
            return std::nullopt;
        }

        const json assetMetadata{ LoadJsonFromFile(assetMetaPath) };
        if (assetMetadata.empty())
        {
            IG_LOG(StaticMeshLoader, Error, "Failed to load asset metadata from {}.", assetMetaPath.string());
            return std::nullopt;
        }

        AssetInfo assetInfo{};
        StaticMeshLoadDesc loadDesc{};
        assetMetadata >> loadDesc >> assetInfo;

        if (assetInfo.Guid != guid)
        {
            IG_LOG(StaticMeshLoader, Error, "Asset guid does not match. Expected: {}, Found: {}",
                   guid.str(), assetInfo.Guid.str());
            return std::nullopt;
        }

        if (assetInfo.Type != EAssetType::StaticMesh)
        {
            IG_LOG(StaticMeshLoader, Error, "Asset type does not match. Expected: {}, Found: {}",
                   magic_enum::enum_name(EAssetType::StaticMesh),
                   magic_enum::enum_name(assetInfo.Type));
            return std::nullopt;
        }

        if (loadDesc.NumVertices == 0 ||
            loadDesc.NumIndices == 0 ||
            loadDesc.CompressedVerticesSizeInBytes == 0 ||
            loadDesc.CompressedIndicesSizeInBytes == 0)
        {
            IG_LOG(StaticMeshLoader, Error,
                   "Load config has invalid values. \n * NumVertices: {}\n * NumIndices: {}\n * CompressedVerticesSizeInBytes: {}\n * CompressedIndicesSizeInBytes: {}",
                   loadDesc.NumVertices, loadDesc.NumIndices, loadDesc.CompressedVerticesSizeInBytes, loadDesc.CompressedIndicesSizeInBytes);
            return std::nullopt;
        }

        std::vector<uint8_t> blob = LoadBlobFromFile(assetPath);
        if (blob.empty())
        {
            IG_LOG(StaticMeshLoader, Error, "Static Mesh {} blob is empty.", assetPath.string());
            return std::nullopt;
        }

        const size_t expectedBlobSize = loadDesc.CompressedVerticesSizeInBytes + loadDesc.CompressedIndicesSizeInBytes;
        if (blob.size() != expectedBlobSize)
        {
            IG_LOG(StaticMeshLoader, Error, "Static Mesh blob size does not match. Expected: {}, Found: {}", expectedBlobSize, blob.size());
            return std::nullopt;
        }

        GpuBufferDesc vertexBufferDesc{};
        vertexBufferDesc.AsStructuredBuffer<StaticMeshVertex>(loadDesc.NumVertices);
        vertexBufferDesc.DebugName = String(std::format("{}_Vertices", guid.str()));
        if (vertexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedVerticesSizeInBytes)
        {
            IG_LOG(StaticMeshLoader, Error, "Compressed vertices size exceed expected vertex buffer size. Compressed Size: {} bytes, Expected Vertex Buffer Size: {} bytes",
                   loadDesc.CompressedVerticesSizeInBytes,
                   vertexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        GpuBufferDesc indexBufferDesc{};
        indexBufferDesc.AsIndexBuffer<uint32_t>(loadDesc.NumIndices);
        indexBufferDesc.DebugName = String(std::format("{}_Indices", guid.str()));
        if (indexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedIndicesSizeInBytes)
        {
            IG_LOG(StaticMeshLoader, Error, "Compressed indices size exceed expected index buffer size. Compressed Size: {} bytes, Expected Index Buffer Size: {} bytes",
                   loadDesc.CompressedIndicesSizeInBytes,
                   indexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        std::optional<GpuBuffer> vertexBuffer = renderDevice.CreateBuffer(vertexBufferDesc);
        if (!vertexBuffer)
        {
            IG_LOG(StaticMeshLoader, Error, "Failed to create vertex buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        auto vertexBufferSrv = gpuViewManager.RequestShaderResourceView(*vertexBuffer);
        if (!vertexBufferSrv)
        {
            IG_LOG(StaticMeshLoader, Error, "Failed to create vertex buffer shader resource view of {}.", assetPath.string());
            return std::nullopt;
        }

        std::optional<GpuBuffer> indexBuffer = renderDevice.CreateBuffer(indexBufferDesc);
        if (!indexBuffer)
        {
            IG_LOG(StaticMeshLoader, Error, "Failed to create index buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadDesc.NumVertices,
                                                                sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBuffer);
            }
            else
            {
                IG_LOG(StaticMeshLoader, Error, "Failed to decode vertex buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

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
            }
            else
            {
                IG_LOG(StaticMeshLoader, Error, "Failed to decode index buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

        verticesUploadSync->WaitOnCpu();
        indicesUploadSync->WaitOnCpu();

        IG_LOG(StaticMeshLoader, Info,
               "Successfully load static mesh asset {}, which from resource {}. Elapsed: {} ms",
               assetPath.string(), assetInfo.VirtualPath.ToStringView(), tempTimer.End());

        return StaticMesh{
            { assetInfo, loadDesc },
            { handleManager, std::move(*vertexBuffer) },
            std::move(vertexBufferSrv),
            { handleManager, std::move(*indexBuffer) }
        };
    }
} // namespace ig