#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/Vertex.h"

namespace ig
{
    class RenderContext;

    class MeshStorage final
    {
      public:
        template <typename Ty>
        struct Space
        {
            GpuStorage::Allocation Allocation;
        };

        struct Statistics
        {
            Size StaticMeshVertexStorageUsage;
            Size StaticMeshVertexStorageSize;
            Size NumStaticMeshVertices;
            Size VertexIndexStorageUsage;
            Size VertexIndexStorageSize;
            Size NumVertexIndices;
        };

      public:
        explicit MeshStorage(RenderContext& renderContext);
        MeshStorage(const MeshStorage&) = delete;
        MeshStorage(MeshStorage&&) = delete;
        ~MeshStorage();

        MeshStorage& operator=(const MeshStorage&) = delete;
        MeshStorage& operator=(MeshStorage&&) = delete;

        Handle<MeshStorage::Space<VertexSM>> CreateStaticMeshVertexSpace(const U32 numVertices);
        Handle<MeshStorage::Space<U32>> CreateVertexIndexSpace(const U32 numIndices);

        void Destroy(const Handle<MeshStorage::Space<VertexSM>> staticMeshVertexSpace);
        void Destroy(const Handle<MeshStorage::Space<U32>> vertexIndexSpace);

        [[nodiscard]] const Space<VertexSM>* Lookup(const Handle<MeshStorage::Space<VertexSM>> handle) const;
        [[nodiscard]] const Space<U32>* Lookup(const Handle<MeshStorage::Space<U32>> handle) const;

        void PreRender(const LocalFrameIndex localFrameIdx);

        [[nodiscard]] Handle<GpuBuffer> GetStaticMeshVertexStorageBuffer() const noexcept { return staticMeshVertexGpuStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetStaticMeshVertexStorageSrv() const noexcept { return staticMeshVertexGpuStorage.GetShaderResourceView(); }
        [[nodiscard]] GpuFence& GetStaticMeshVertexStorageFence() noexcept { return staticMeshVertexGpuStorage.GetStorageFence(); }
        [[nodiscard]] Handle<GpuBuffer> GetIndexStorageBuffer() const noexcept { return vertexIndexGpuStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetIndexStorageSrv() const noexcept { return vertexIndexGpuStorage.GetShaderResourceView(); }
        [[nodiscard]] GpuFence& GetIndexStorageFence() noexcept { return vertexIndexGpuStorage.GetStorageFence(); }

        [[nodiscard]] Statistics GetStatistics() const;

      private:
        LocalFrameIndex currentLocalFrameIdx = 0;
        RenderContext& renderContext;
        DeferredResourceManagePackage<Space<VertexSM>> staticMeshVertexSpacePackage;
        GpuStorage staticMeshVertexGpuStorage;
        DeferredResourceManagePackage<Space<U32>> vertexIndexSpacePackage;
        GpuStorage vertexIndexGpuStorage;
    };
} // namespace ig
