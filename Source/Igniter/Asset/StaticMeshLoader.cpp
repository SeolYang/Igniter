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

IG_DECLARE_LOG_CATEGORY(StaticMeshLoader);

namespace ig
{
    StaticMeshLoader::StaticMeshLoader(RenderContext& renderContext, AssetManager& assetManager)
        : renderContext(renderContext)
        , assetManager(assetManager)
    {
    }

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

        if (loadDesc.NumVertices == 0 || loadDesc.CompressedVerticesSizeInBytes == 0 || loadDesc.NumLods == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidArguments>();
        }

        Size expectedSumCompressedLodIndicesSize = 0;
        for (Index lod = 0; lod < loadDesc.NumLods; ++lod)
        {
            if (loadDesc.NumIndicesPerLod[lod] == 0 || loadDesc.CompressedIndicesSizePerLod[lod] == 0)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::InvalidArguments>();
            }

            expectedSumCompressedLodIndicesSize += loadDesc.CompressedIndicesSizePerLod[lod];
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());
        if (!fs::exists(assetPath))
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FileDoesNotExists>();
        }

        Vector<uint8_t> blob = LoadBlobFromFile(assetPath);
        if (blob.empty())
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::EmptyBlob>();
        }

        const Size expectedBlobSize = loadDesc.CompressedVerticesSizeInBytes + expectedSumCompressedLodIndicesSize;
        if (blob.size() != expectedBlobSize)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::BlobSizeMismatch>();
        }

        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        GpuUploader& gpuUploader{renderContext.GetGpuUploader()};
        MeshStorage& meshStorage = Engine::GetMeshStorage();
        const Handle<MeshStorage::Space<VertexSM>> vertexSpace = meshStorage.CreateStaticMeshVertexSpace(loadDesc.NumVertices);
        if (!vertexSpace)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexSpace>();
        }

        const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage.Lookup(vertexSpace);
        IG_CHECK(vertexSpacePtr->Allocation.NumElements == loadDesc.NumVertices);
        bool bVertexBufferDecodeSucceed = false;
        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexSpacePtr->Allocation.AllocSize);
        {
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(),
                                                                loadDesc.NumVertices,
                                                                sizeof(VertexSM),
                                                                blob.data(),
                                                                loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                const Handle<GpuBuffer> vertexStorageBuffer = meshStorage.GetStaticMeshVertexStorageBuffer();
                GpuBuffer* vertexStorageBufferPtr = renderContext.Lookup(vertexStorageBuffer);
                IG_CHECK(vertexStorageBufferPtr != nullptr);
                verticesUploadCtx.CopyBuffer(0, vertexSpacePtr->Allocation.AllocSize,
                                             *vertexStorageBufferPtr, vertexSpacePtr->Allocation.Offset);
                bVertexBufferDecodeSucceed = true;
            }
        }
        std::optional<GpuSyncPoint> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        // 여기를 NumLOD에 맞게 만들어 줘야함
        bool bIndexBufferDecodeSucceed = false;
        Array<Handle<MeshStorage::Space<U32>>, StaticMesh::kMaxNumLods> lodIndexSpaces{};
        Array<Option<GpuSyncPoint>, StaticMesh::kMaxNumLods> lodIndicesUploadSyncs{};
        for (Index lod = 0; lod < loadDesc.NumLods; ++lod)
        {
            lodIndexSpaces[lod] = meshStorage.CreateVertexIndexSpace(loadDesc.NumIndicesPerLod[lod]);
            if (!lodIndexSpaces[lod])
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedCreateVertexIndexSpace>();
            }
        }

        const Handle<GpuBuffer> vertexIndexStorageBuffer = meshStorage.GetIndexStorageBuffer();
        GpuBuffer* vertexIndexStorageBufferPtr = renderContext.Lookup(vertexIndexStorageBuffer);
        IG_CHECK(vertexIndexStorageBufferPtr != nullptr);
        Size compressedIndicesOffset = loadDesc.CompressedVerticesSizeInBytes;
        for (Index lod = 0; lod < loadDesc.NumLods; ++lod)
        {
            const MeshStorage::Space<U32>* vertexIndexSpacePtr = meshStorage.Lookup(lodIndexSpaces[lod]);
            IG_CHECK(vertexIndexSpacePtr->Allocation.NumElements == loadDesc.NumIndicesPerLod[lod]);
            UploadContext indicesUploadCtx = gpuUploader.Reserve(vertexIndexSpacePtr->Allocation.AllocSize);
            {
                const int decodeResult = meshopt_decodeIndexBuffer<U32>(
                    reinterpret_cast<U32*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                    loadDesc.NumIndicesPerLod[lod],
                    blob.data() + compressedIndicesOffset, loadDesc.CompressedIndicesSizePerLod[lod]);

                if (decodeResult == 0)
                {
                    indicesUploadCtx.CopyBuffer(0, vertexIndexSpacePtr->Allocation.AllocSize,
                                                *vertexIndexStorageBufferPtr, vertexIndexSpacePtr->Allocation.Offset);
                    bIndexBufferDecodeSucceed = true;
                    compressedIndicesOffset += loadDesc.CompressedIndicesSizePerLod[lod];
                }
                else
                {
                    bIndexBufferDecodeSucceed = false;
                    break;
                }
            }
            lodIndicesUploadSyncs[lod] = gpuUploader.Submit(indicesUploadCtx);
            IG_CHECK(lodIndicesUploadSyncs[lod]);
        }

        verticesUploadSync->WaitOnCpu();
        for (Index lod = 0; lod < loadDesc.NumLods; ++lod)
        {
            lodIndicesUploadSyncs[lod]->WaitOnCpu();
        }

        if (!bVertexBufferDecodeSucceed)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedDecodeVertexBuffer>();
        }

        if (!bIndexBufferDecodeSucceed)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedDecodeIndexBuffer>();
        }

        return MakeSuccess<StaticMesh, EStaticMeshLoadStatus>(renderContext, assetManager, desc, vertexSpace, loadDesc.NumLods, lodIndexSpaces);
    }
} // namespace ig
