#include "Igniter/Igniter.h"
#include "Igniter/Render/MeshStorage.h"

namespace ig
{
    MeshStorage::MeshStorage(RenderContext& renderContext) :
        renderContext(renderContext),
        staticMeshVertexGpuStorage(renderContext, GpuStorageDesc{"StaticMeshVertexStorage"_fs, static_cast<U32>(sizeof(VertexSM)), 8192u}),
        vertexIndexGpuStorage(renderContext, GpuStorageDesc{"VertexIndexStorage"_fs, static_cast<U32>(sizeof(U32)), 16u * 8192u}) {}

    MeshStorage::~MeshStorage()
    {
        for (LocalFrameIndex idx = 0; idx < NumFramesInFlight; ++idx)
        {
            PreRender(idx);
        }
    }

    MeshStorage::Handle<VertexSM> MeshStorage::CreateStaticMeshVertexSpace(const U32 numVertices)
    {
        IG_CHECK(numVertices > 0);
        ReadWriteLock storageMutex{staticMeshVertexSpacePackage.StorageMutex};

        const Space<VertexSM> newVertexSpace{staticMeshVertexGpuStorage.Allocate(numVertices)};
        if (!newVertexSpace.Allocation.IsValid())
        {
            return MeshStorage::Handle<VertexSM>{};
        }

        return staticMeshVertexSpacePackage.Storage.Create(newVertexSpace);
    }

    MeshStorage::Handle<unsigned> MeshStorage::CreateVertexIndexSpace(const U32 numIndices)
    {
        IG_CHECK(numIndices > 0);
        ReadWriteLock storageMutex{vertexIndexSpacePackage.StorageMutex};

        const Space<U32> newIndexSpace{vertexIndexGpuStorage.Allocate(numIndices)};
        if (!newIndexSpace.Allocation.IsValid())
        {
            return MeshStorage::Handle<U32>{};
        }

        return vertexIndexSpacePackage.Storage.Create(newIndexSpace);
    }

    void MeshStorage::Destroy(const Handle<VertexSM> staticMeshVertexSpace)
    {
        IG_CHECK(currentLocalFrameIdx < NumFramesInFlight);
        if (!staticMeshVertexSpace)
        {
            return;
        }

        ScopedLock packageLock{staticMeshVertexSpacePackage.StorageMutex, staticMeshVertexSpacePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
        staticMeshVertexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(staticMeshVertexSpace);
        staticMeshVertexSpacePackage.Storage.MarkAsDestroy(staticMeshVertexSpace);
    }

    void MeshStorage::Destroy(const Handle<U32> vertexIndexSpace)
    {
        IG_CHECK(currentLocalFrameIdx < NumFramesInFlight);
        if (!vertexIndexSpace)
        {
            return;
        }

        ScopedLock packageLock{vertexIndexSpacePackage.StorageMutex, vertexIndexSpacePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
        vertexIndexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].emplace_back(vertexIndexSpace);
        vertexIndexSpacePackage.Storage.MarkAsDestroy(vertexIndexSpace);
    }

    const MeshStorage::Space<VertexSM>* MeshStorage::Lookup(const Handle<VertexSM> handle) const
    {
        ReadOnlyLock storageMutex{staticMeshVertexSpacePackage.StorageMutex};
        return staticMeshVertexSpacePackage.Storage.Lookup(handle);
    }

    const MeshStorage::Space<unsigned>* MeshStorage::Lookup(const Handle<U32> handle) const
    {
        ReadOnlyLock storageMutex{vertexIndexSpacePackage.StorageMutex};
        return vertexIndexSpacePackage.Storage.Lookup(handle);
    }

    void MeshStorage::PreRender(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        currentLocalFrameIdx = localFrameIdx;

        {
            ScopedLock packageLock{staticMeshVertexSpacePackage.StorageMutex, staticMeshVertexSpacePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            for (const auto handle : staticMeshVertexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx])
            {
                const Space<VertexSM>* space = staticMeshVertexSpacePackage.Storage.LookupUnsafe(handle);
                IG_CHECK(space != nullptr);
                staticMeshVertexGpuStorage.Deallocate(space->Allocation);
                staticMeshVertexSpacePackage.Storage.Destroy(handle);
            }
            staticMeshVertexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].clear();
        }

        {
            ScopedLock packageLock{vertexIndexSpacePackage.StorageMutex, vertexIndexSpacePackage.DeferredDestroyPendingListMutex.Resources[currentLocalFrameIdx]};
            for (const auto handle : vertexIndexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx])
            {
                const Space<U32>* space = vertexIndexSpacePackage.Storage.LookupUnsafe(handle);
                IG_CHECK(space != nullptr);
                vertexIndexGpuStorage.Deallocate(space->Allocation);
                vertexIndexSpacePackage.Storage.Destroy(handle);
            }
            vertexIndexSpacePackage.DeferredDestroyPendingList.Resources[currentLocalFrameIdx].clear();
        }
    }

    MeshStorage::Statistics MeshStorage::GetStatistics() const
    {
        Statistics stats{};
        {
            ReadOnlyLock storageMutex{staticMeshVertexSpacePackage.StorageMutex};
            stats.StaticMeshVertexStorageUsage = staticMeshVertexGpuStorage.GetAllocatedSize();
            stats.StaticMeshVertexStorageSize = staticMeshVertexGpuStorage.GetBufferSize();
            stats.NumStaticMeshVertices = staticMeshVertexGpuStorage.GetNumAllocatedElements();
        }

        {
            ReadOnlyLock storageMutex{vertexIndexSpacePackage.StorageMutex};
            stats.VertexIndexStorageUsage = vertexIndexGpuStorage.GetAllocatedSize();
            stats.VertexIndexStorageSize = vertexIndexGpuStorage.GetBufferSize();
            stats.NumVertexIndices = vertexIndexGpuStorage.GetNumAllocatedElements();
        }

        return stats;
    }
} // namespace ig
