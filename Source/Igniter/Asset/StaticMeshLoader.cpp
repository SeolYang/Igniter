#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/Render/Vertex.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMeshLoader.h"

IG_DEFINE_LOG_CATEGORY(StaticMeshLoader);

namespace ig
{
    StaticMeshLoader::StaticMeshLoader(RenderContext& renderContext, AssetManager& assetManager) :
        renderContext(renderContext),
        assetManager(assetManager) {}

    Result<StaticMesh, EStaticMeshLoadStatus> StaticMeshLoader::Load(const StaticMesh::Desc& desc) const
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

        MeshStorage& meshStorage = Engine::GetMeshStorage();
        const MeshStorage::Handle<VertexSM> vertexSpace = meshStorage.CreateStaticMeshVertexSpace(loadDesc.NumVertices);
        if (!vertexSpace)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexSpace>();
        }

        const MeshStorage::Handle<U32> vertexIndexSpace = meshStorage.CreateVertexIndexSpace(loadDesc.NumIndices);
        if (!vertexIndexSpace)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexIndexSpace>();
        }

        const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage.Lookup(vertexSpace);
        const MeshStorage::Space<U32>* vertexIndexSpacePtr = meshStorage.Lookup(vertexIndexSpace);
        IG_CHECK(vertexSpacePtr != nullptr && vertexIndexSpacePtr != nullptr);
        IG_CHECK(vertexSpacePtr->Allocation.NumElements == loadDesc.NumVertices);
        IG_CHECK(vertexIndexSpacePtr->Allocation.NumElements == loadDesc.NumIndices);

        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};
        bool bVertexBufferDecodeSucceed = false;
        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexSpacePtr->Allocation.AllocSize);
        {
            constexpr size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(),
                                                                loadDesc.NumVertices,
                                                                sizeof(VertexSM),
                                                                blob.data() + compressedVerticesOffset,
                                                                loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                const RenderHandle<GpuBuffer> vertexStorageBuffer = meshStorage.GetStaticMeshVertexStorageBuffer();
                GpuBuffer* vertexStorageBufferPtr = renderContext.Lookup(vertexStorageBuffer);
                IG_CHECK(vertexStorageBufferPtr != nullptr);
                verticesUploadCtx.CopyBuffer(0, vertexSpacePtr->Allocation.AllocSize,
                                             *vertexStorageBufferPtr, vertexSpacePtr->Allocation.Offset);
                bVertexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSyncPoint> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        bool bIndexBufferDecodeSucceed = false;
        UploadContext indicesUploadCtx = gpuUploader.Reserve(vertexIndexSpacePtr->Allocation.AllocSize);
        {
            const size_t compressedIndicesOffset = loadDesc.CompressedVerticesSizeInBytes;
            const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                                                                         loadDesc.NumIndices, blob.data() + compressedIndicesOffset, loadDesc.CompressedIndicesSizeInBytes);

            if (decodeResult == 0)
            {
                const RenderHandle<GpuBuffer> vertexIndexStorageBuffer = meshStorage.GetVertexIndexStorageBuffer();
                GpuBuffer* vertexIndexStorageBufferPtr = renderContext.Lookup(vertexIndexStorageBuffer);
                IG_CHECK(vertexIndexStorageBufferPtr != nullptr);
                indicesUploadCtx.CopyBuffer(0, vertexIndexSpacePtr->Allocation.AllocSize,
                                            *vertexIndexStorageBufferPtr, vertexIndexSpacePtr->Allocation.Offset);
                bIndexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSyncPoint> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

        const ManagedAsset<Material> material{assetManager.Load<Material>(loadDesc.MaterialGuid)};
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
            StaticMesh{renderContext, assetManager, desc, vertexSpace, vertexIndexSpace, material});
    }
} // namespace ig
