#include "Igniter/Igniter.h"
#include "Igniter/Render/UnifiedMeshStorage.h"

namespace ig
{
    UnifiedMeshStorage::UnifiedMeshStorage(RenderContext& renderCtx)
        : renderCtx(&renderCtx)
        , vertexStorage(renderCtx, GpuStorageDesc{
            .DebugName = "VertexStorage(UnifiedMeshStorage)"_fs,
            .ElementSize = sizeof(U32),
            .NumInitElements = 65'536Ui32,
            .Flags = EGpuStorageFlags::None
        })
        , indexStorage(renderCtx, GpuStorageDesc{
            .DebugName = "IndexStorage(UnifiedMeshStorage)"_fs,
            .ElementSize = sizeof(U32),
            .NumInitElements = 32'768Ui32,
            .Flags = EGpuStorageFlags::None
        })
        , triangleStorage(renderCtx, GpuStorageDesc{
            .DebugName = "TriangleStorage(UnifiedMeshStorage)"_fs,
            .ElementSize = sizeof(U32),
            .NumInitElements = 65'536Ui32 / 3Ui32,
            .Flags = EGpuStorageFlags::None
        })
        , meshletStorage(renderCtx, GpuStorageDesc{
            .DebugName = "MeshletStorage(UnifiedMeshStorage)"_fs,
            .ElementSize = sizeof(Meshlet),
            .NumInitElements = 8192,
            .Flags = EGpuStorageFlags::None
        })
    {}

    UnifiedMeshStorage::~UnifiedMeshStorage()
    {
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            PreRender(localFrameIdx);
        }

        IG_CHECK(numAllocVertices == 0);
        IG_CHECK(numAllocIndices == 0);
        IG_CHECK(numAllocTriangles == 0);
        IG_CHECK(numAllocMeshlets == 0);
    }

    Handle<MeshVertex> UnifiedMeshStorage::AllocateVertices(const Size numVertices, const Size numDwordsPerVertex)
    {
        if (numVertices == 0)
        {
            return {};
        }

        if ((numDwordsPerVertex % 4) != 0)
        {
            return {};
        }

        ReadWriteLock storageLock{vertexDeferredManagePackage.StorageMutex};
        const GpuStorage::Allocation newAlloc = vertexStorage.Allocate(numVertices * numDwordsPerVertex);
        if (!newAlloc.IsValid())
        {
            return {};
        }

        const Handle<MeshVertexAllocation> newHandle =
            vertexDeferredManagePackage.Storage.Create(newAlloc, numVertices, sizeof(U32) * numDwordsPerVertex);
        if (!newHandle)
        {
            vertexStorage.Deallocate(newAlloc);
            return {};
        }

        numAllocVertices += numVertices;
        return Handle<MeshVertex>{newHandle.Value};
    }

    Handle<MeshIndex> UnifiedMeshStorage::AllocateIndices(const Size numIndices)
    {
        if (numIndices == 0)
        {
            return {};
        }

        ReadWriteLock storageLock{indexDeferredManagedPackage.StorageMutex};
        const GpuStorage::Allocation newAlloc = indexStorage.Allocate(numIndices);
        if (!newAlloc.IsValid())
        {
            return {};
        }

        const Handle<GpuStorage::Allocation> newHandle =
            indexDeferredManagedPackage.Storage.Create(newAlloc);
        if (!newHandle)
        {
            indexStorage.Deallocate(newAlloc);
            return {};
        }

        numAllocIndices += numIndices;
        return Handle<MeshIndex>{newHandle.Value};
    }

    Handle<MeshTriangle> UnifiedMeshStorage::AllocateTriangles(const Size numTriangles)
    {
        if (numTriangles == 0)
        {
            return {};
        }

        ReadWriteLock storageLock{triangleDeferredManagePackage.StorageMutex};
        const GpuStorage::Allocation newAlloc = triangleStorage.Allocate(numTriangles);
        if (!newAlloc.IsValid())
        {
            return {};
        }

        const Handle<GpuStorage::Allocation> newHandle =
            triangleDeferredManagePackage.Storage.Create(newAlloc);
        if (!newHandle)
        {
            triangleStorage.Deallocate(newAlloc);
            return {};
        }

        numAllocTriangles += numTriangles;
        return Handle<MeshTriangle>{newHandle.Value};
    }

    Handle<Meshlet> UnifiedMeshStorage::AllocateMeshlets(const Size numMeshlets)
    {
        if (numMeshlets == 0)
        {
            return {};
        }

        ReadWriteLock storageLock{meshletDeferredManagePackage.StorageMutex};
        const GpuStorage::Allocation newAlloc = meshletStorage.Allocate(numMeshlets);
        if (!newAlloc.IsValid())
        {
            return {};
        }

        const Handle<GpuStorage::Allocation> newHandle =
            meshletDeferredManagePackage.Storage.Create(newAlloc);
        if (!newHandle)
        {
            meshletStorage.Deallocate(newAlloc);
            return {};
        }

        numAllocMeshlets += numMeshlets;
        return Handle<Meshlet>{newHandle.Value};
    }

    void UnifiedMeshStorage::Deallocate(const Handle<MeshVertex> handle)
    {
        if (!handle)
        {
            return;
        }

        UniqueLock deferredListLock{vertexDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]};
        vertexDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].emplace_back(handle.Value);
    }

    void UnifiedMeshStorage::Deallocate(const Handle<MeshIndex> handle)
    {
        if (!handle)
        {
            return;
        }

        UniqueLock deferredListLock{indexDeferredManagedPackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]};
        indexDeferredManagedPackage.DeferredDestroyPendingList[currentLocalFrameIdx].emplace_back(handle.Value);
    }

    void UnifiedMeshStorage::Deallocate(const Handle<MeshTriangle> handle)
    {
        if (!handle)
        {
            return;
        }

        UniqueLock deferredListLock{triangleDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]};
        triangleDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].emplace_back(handle.Value);
    }

    void UnifiedMeshStorage::Deallocate(const Handle<Meshlet> handle)
    {
        if (!handle)
        {
            return;
        }

        UniqueLock deferredListLock{meshletDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]};
        meshletDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].emplace_back(handle.Value);
    }

    const MeshVertexAllocation* UnifiedMeshStorage::Lookup(const Handle<MeshVertex> handle) const noexcept
    {
        if (!handle)
        {
            return nullptr;
        }

        ReadOnlyLock storageLock{vertexDeferredManagePackage.StorageMutex};
        return vertexDeferredManagePackage.Storage.Lookup(Handle<MeshVertexAllocation>{handle.Value});
    }

    const GpuStorage::Allocation* UnifiedMeshStorage::Lookup(const Handle<MeshIndex> handle) const noexcept
    {
        if (!handle)
        {
            return nullptr;
        }

        ReadOnlyLock storageLock{indexDeferredManagedPackage.StorageMutex};
        return indexDeferredManagedPackage.Storage.Lookup(Handle<GpuStorage::Allocation>{handle.Value});
    }

    const GpuStorage::Allocation* UnifiedMeshStorage::Lookup(const Handle<MeshTriangle> handle) const noexcept
    {
        if (!handle)
        {
            return nullptr;
        }

        ReadOnlyLock storageLock{triangleDeferredManagePackage.StorageMutex};
        return triangleDeferredManagePackage.Storage.Lookup(Handle<GpuStorage::Allocation>{handle.Value});
    }

    const GpuStorage::Allocation* UnifiedMeshStorage::Lookup(const Handle<Meshlet> handle) const noexcept
    {
        if (!handle)
        {
            return nullptr;
        }

        ReadOnlyLock storageLock{meshletDeferredManagePackage.StorageMutex};
        return meshletDeferredManagePackage.Storage.Lookup(Handle<GpuStorage::Allocation>{handle.Value});
    }

    void UnifiedMeshStorage::PreRender(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        currentLocalFrameIdx = localFrameIdx;

        {
            ScopedLock packageLock{
                vertexDeferredManagePackage.StorageMutex,
                vertexDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]
            };
            for (const Handle<MeshVertexAllocation> handle : vertexDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx])
            {
                const MeshVertexAllocation* alloc = vertexDeferredManagePackage.Storage.Lookup(handle);
                IG_CHECK(alloc != nullptr);
                vertexStorage.Deallocate(alloc->Alloc);
                vertexDeferredManagePackage.Storage.Destroy(handle);

                IG_CHECK(numAllocVertices > 0);
                numAllocVertices -= alloc->NumVertices;
            }
            vertexDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].clear();
        }

        {
            ScopedLock packageLock{
                indexDeferredManagedPackage.StorageMutex,
                indexDeferredManagedPackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]
            };
            for (const Handle<GpuStorage::Allocation> handle : indexDeferredManagedPackage.DeferredDestroyPendingList[currentLocalFrameIdx])
            {
                const GpuStorage::Allocation* alloc = indexDeferredManagedPackage.Storage.Lookup(handle);
                IG_CHECK(alloc != nullptr);
                indexStorage.Deallocate(*alloc);
                indexDeferredManagedPackage.Storage.Destroy(handle);

                IG_CHECK(numAllocIndices > 0);
                numAllocIndices -= alloc->NumElements;
            }
            indexDeferredManagedPackage.DeferredDestroyPendingList[currentLocalFrameIdx].clear();
        }

        {
            ScopedLock packageLock{
                triangleDeferredManagePackage.StorageMutex,
                triangleDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]
            };
            for (const Handle<GpuStorage::Allocation> handle : triangleDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx])
            {
                const GpuStorage::Allocation* alloc = triangleDeferredManagePackage.Storage.Lookup(handle);
                IG_CHECK(alloc != nullptr);
                triangleStorage.Deallocate(*alloc);
                triangleDeferredManagePackage.Storage.Destroy(handle);

                IG_CHECK(numAllocTriangles > 0);
                numAllocTriangles -= alloc->NumElements;
            }
            triangleDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].clear();
        }

        {
            ScopedLock packageLock{
                meshletDeferredManagePackage.StorageMutex,
                meshletDeferredManagePackage.DeferredDestroyPendingListMutex[currentLocalFrameIdx]
            };
            for (const Handle<GpuStorage::Allocation> handle : meshletDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx])
            {
                const GpuStorage::Allocation* alloc = meshletDeferredManagePackage.Storage.Lookup(handle);
                IG_CHECK(alloc != nullptr);
                meshletStorage.Deallocate(*alloc);
                meshletDeferredManagePackage.Storage.Destroy(handle);

                IG_CHECK(numAllocMeshlets > 0);
                numAllocMeshlets -= alloc->NumElements;
            }
            meshletDeferredManagePackage.DeferredDestroyPendingList[currentLocalFrameIdx].clear();
        }
    }
} // namespace ig
