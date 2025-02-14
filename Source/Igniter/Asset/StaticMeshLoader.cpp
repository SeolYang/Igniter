#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/Render/Vertex.h"
#include "Igniter/Render/GpuUploader.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Mesh.h"
#include "Igniter/Render/UnifiedMeshStorage.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMeshLoader.h"

IG_DECLARE_LOG_CATEGORY(StaticMeshLoaderLog);

IG_DEFINE_LOG_CATEGORY(StaticMeshLoaderLog);

namespace ig
{
    StaticMeshLoader::StaticMeshLoader(RenderContext& renderContext, AssetManager& assetManager)
        : renderContext(renderContext)
        , assetManager(assetManager)
    {}

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

        if (loadDesc.NumVertices == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroNumVertices>();
        }

        if (loadDesc.CompressedVerticesSize == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroCompressedVerticesSize>();
        }

        if (loadDesc.NumLevelOfDetails == 0)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroNumLevelOfDetails>();
        }

        if (loadDesc.NumLevelOfDetails > Mesh::kMaxMeshLevelOfDetails)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::ExceededNumLevelOfDetails>();
        }

        Size expectedNumIndices = 0;
        Size expectedNumTriangles = 0;
        Size expectedNumMeshlets = 0;
        for (U8 lod = 0; lod < loadDesc.NumLevelOfDetails; ++lod)
        {
            if (loadDesc.NumMeshletVertexIndices[lod] == 0)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroNumMeshletVertexIndices>();
            }
            if (loadDesc.NumMeshletTriangles[lod] == 0)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroNumMeshletTriangles>();
            }
            if (loadDesc.NumMeshlets[lod] == 0)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::ZeroNumMeshlets>();
            }
            expectedNumIndices += loadDesc.NumMeshletVertexIndices[lod];
            expectedNumTriangles += loadDesc.NumMeshletTriangles[lod];
            expectedNumMeshlets += loadDesc.NumMeshlets[lod];
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

        const Size expectedBlobSize = loadDesc.CompressedVerticesSize +
            sizeof(U32) * expectedNumIndices +
            sizeof(U32) * expectedNumTriangles +
            sizeof(Meshlet) * expectedNumMeshlets;
        if (blob.size() != expectedBlobSize)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::BlobSizeMismatch>();
        }

        meshopt_encodeVertexVersion(0);

        GpuUploader& gpuUploader{renderContext.GetNonFrameCriticalGpuUploader()};
        UnifiedMeshStorage& unifiedMeshStorage = renderContext.GetUnifiedMeshStorage();

        const auto kDeleter = [&unifiedMeshStorage](Mesh* mesh)
        {
            IG_CHECK(mesh != nullptr);
            unifiedMeshStorage.Deallocate(mesh->VertexStorageAlloc);
            for (U8 lod = 0; lod < Mesh::kMaxMeshLevelOfDetails; ++lod)
            {
                unifiedMeshStorage.Deallocate(mesh->LevelOfDetails[lod].IndexStorageAlloc);
                unifiedMeshStorage.Deallocate(mesh->LevelOfDetails[lod].TriangleStorageAlloc);
                unifiedMeshStorage.Deallocate(mesh->LevelOfDetails[lod].MeshletStorageAlloc);
            }
        };
        /* Ptr의 RAII를 통해 Load 실패 시 안전하게 자원을 해제; 성공 시 Release를 통해 자원 해제 비활성화 */
        Mesh newMesh;
        Ptr<Mesh, decltype(kDeleter)> meshGuard{&newMesh, kDeleter};

        newMesh.NumLevelOfDetails = loadDesc.NumLevelOfDetails;
        newMesh.BoundingBox = loadDesc.BoundingBox;
        newMesh.VertexStorageAlloc = unifiedMeshStorage.AllocateVertices<VertexSM>(loadDesc.NumVertices);
        if (!newMesh.VertexStorageAlloc)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedAllocateVertexSpace>();
        }

        for (U8 lod = 0; lod < newMesh.NumLevelOfDetails; ++lod)
        {
            MeshLod& meshLod = newMesh.LevelOfDetails[lod];
            meshLod.IndexStorageAlloc = unifiedMeshStorage.AllocateIndices(loadDesc.NumMeshletVertexIndices[lod]);
            if (!meshLod.IndexStorageAlloc)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedAllocateIndexSpace>();
            }

            meshLod.TriangleStorageAlloc = unifiedMeshStorage.AllocateTriangles(loadDesc.NumMeshletTriangles[lod]);
            if (!meshLod.TriangleStorageAlloc)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedAllocateTriangleSpace>();
            }

            meshLod.MeshletStorageAlloc = unifiedMeshStorage.AllocateMeshlets(loadDesc.NumMeshlets[lod]);
            if (!meshLod.MeshletStorageAlloc)
            {
                return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedAllocateTriangleSpace>();
            }
        }
        const MeshVertexAllocation* meshVertexAllocPtr = unifiedMeshStorage.Lookup(newMesh.VertexStorageAlloc);
        IG_CHECK(meshVertexAllocPtr != nullptr);
        IG_CHECK(meshVertexAllocPtr->NumVertices == loadDesc.NumVertices);
        IG_CHECK(meshVertexAllocPtr->SizeOfVertex == sizeof(VertexSM));
        bool bVerticesDecodeSucceed = false;
        UploadContext verticesUploadCtx = gpuUploader.Reserve(meshVertexAllocPtr->Alloc.AllocSize);
        {
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(),
                loadDesc.NumVertices,
                sizeof(VertexSM),
                blob.data(),
                loadDesc.CompressedVerticesSize);
            if (decodeResult == 0)
            {
                GpuBuffer* vertexStorageBufferPtr = renderContext.Lookup(unifiedMeshStorage.GetVertexStorageBuffer());
                IG_CHECK(vertexStorageBufferPtr != nullptr);
                verticesUploadCtx.CopyBuffer(0, meshVertexAllocPtr->Alloc.AllocSize,
                    *vertexStorageBufferPtr, meshVertexAllocPtr->Alloc.Offset);
                bVerticesDecodeSucceed = true;
            }
        }
        GpuSyncPoint verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);

        GpuBuffer* indexStorageBufferPtr = renderContext.Lookup(unifiedMeshStorage.GetIndexStorageBuffer());
        IG_CHECK(indexStorageBufferPtr != nullptr);
        GpuBuffer* triangleStorageBufferPtr = renderContext.Lookup(unifiedMeshStorage.GetTriangleStorageBuffer());
        IG_CHECK(triangleStorageBufferPtr != nullptr);
        GpuBuffer* meshletStorageBufferPtr = renderContext.Lookup(unifiedMeshStorage.GetMeshletStorageBuffer());
        IG_CHECK(meshletStorageBufferPtr != nullptr);

        Vector<GpuSyncPoint> lodUploadSyncPoints{newMesh.NumLevelOfDetails};
        Size blobOffset = loadDesc.CompressedVerticesSize;
        for (U8 lod = 0; lod < newMesh.NumLevelOfDetails; ++lod)
        {
            MeshLod& meshLod = newMesh.LevelOfDetails[lod];

            const GpuStorage::Allocation* indexStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.IndexStorageAlloc);
            IG_CHECK(indexStorageAllocPtr != nullptr);
            IG_CHECK(indexStorageAllocPtr->NumElements == loadDesc.NumMeshletVertexIndices[lod]);
            IG_CHECK(indexStorageAllocPtr->AllocSize == (loadDesc.NumMeshletVertexIndices[lod] * sizeof(U32)));

            const GpuStorage::Allocation* triangleStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.TriangleStorageAlloc);
            IG_CHECK(triangleStorageAllocPtr != nullptr);
            IG_CHECK(triangleStorageAllocPtr->NumElements == loadDesc.NumMeshletTriangles[lod]);
            IG_CHECK(triangleStorageAllocPtr->AllocSize == (loadDesc.NumMeshletTriangles[lod] * sizeof(U32)));

            const GpuStorage::Allocation* meshletStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.MeshletStorageAlloc);
            IG_CHECK(meshletStorageAllocPtr != nullptr);
            IG_CHECK(meshletStorageAllocPtr->NumElements == loadDesc.NumMeshlets[lod]);
            IG_CHECK(meshletStorageAllocPtr->AllocSize == loadDesc.NumMeshlets[lod] * sizeof(Meshlet));

            const Size requiredUploadCtxSize = indexStorageAllocPtr->AllocSize + triangleStorageAllocPtr->AllocSize + meshletStorageAllocPtr->AllocSize;
            Size uploadCtxOffset = 0;
            UploadContext lodUploadCtx = gpuUploader.Reserve(requiredUploadCtxSize);

            std::memcpy(lodUploadCtx.GetOffsettedCpuAddress() + uploadCtxOffset,
                blob.data() + blobOffset,
                indexStorageAllocPtr->AllocSize);
            lodUploadCtx.CopyBuffer(
                uploadCtxOffset,
                indexStorageAllocPtr->AllocSize, *indexStorageBufferPtr, indexStorageAllocPtr->Offset);
            uploadCtxOffset += indexStorageAllocPtr->AllocSize;
            blobOffset += indexStorageAllocPtr->AllocSize;

            std::memcpy(lodUploadCtx.GetOffsettedCpuAddress() + uploadCtxOffset,
                blob.data() + blobOffset,
                triangleStorageAllocPtr->AllocSize);
            lodUploadCtx.CopyBuffer(
                uploadCtxOffset,
                triangleStorageAllocPtr->AllocSize, *triangleStorageBufferPtr, triangleStorageAllocPtr->Offset);
            uploadCtxOffset += triangleStorageAllocPtr->AllocSize;
            blobOffset += triangleStorageAllocPtr->AllocSize;

            std::memcpy(lodUploadCtx.GetOffsettedCpuAddress() + uploadCtxOffset,
                blob.data() + blobOffset,
                meshletStorageAllocPtr->AllocSize);
            lodUploadCtx.CopyBuffer(
                uploadCtxOffset,
                meshletStorageAllocPtr->AllocSize, *meshletStorageBufferPtr, meshletStorageAllocPtr->Offset);
            uploadCtxOffset += meshletStorageAllocPtr->AllocSize;
            blobOffset += meshletStorageAllocPtr->AllocSize;

            IG_CHECK(uploadCtxOffset == requiredUploadCtxSize);
            lodUploadSyncPoints[lod] = gpuUploader.Submit(lodUploadCtx);
        }

        verticesUploadSync.WaitOnCpu();
        for (U8 lod = 0; lod < newMesh.NumLevelOfDetails; ++lod)
        {
            lodUploadSyncPoints[lod].WaitOnCpu();
        }

        if (!bVerticesDecodeSucceed)
        {
            return MakeFail<StaticMesh, EStaticMeshLoadStatus::FailedDecodeVertexBuffer>();
        }

        meshGuard.release();
        return MakeSuccess<StaticMesh, EStaticMeshLoadStatus>(renderContext, assetManager, desc, newMesh);
    }
} // namespace ig
