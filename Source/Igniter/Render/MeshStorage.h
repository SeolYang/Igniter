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

        template <typename Ty>
        using Handle = Handle<Space<Ty>, MeshStorage>;

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
        MeshStorage(MeshStorage&&)      = delete;
        ~MeshStorage();

        MeshStorage& operator=(const MeshStorage&) = delete;
        MeshStorage& operator=(MeshStorage&&)      = delete;

        Handle<VertexSM> CreateStaticMeshVertexSpace(const U32 numVertices);
        Handle<U32>      CreateVertexIndexSpace(const U32 numIndices);

        void Destroy(const Handle<VertexSM> staticMeshVertexSpace);
        void Destroy(const Handle<U32> vertexIndexSpace);

        [[nodiscard]] const Space<VertexSM>* Lookup(const Handle<VertexSM> handle) const;
        [[nodiscard]] const Space<U32>*      Lookup(const Handle<U32> handle) const;

        void PreRender(const LocalFrameIndex localFrameIdx);

        [[nodiscard]] RenderHandle<GpuBuffer> GetStaticMeshVertexStorageBuffer() const noexcept { return staticMeshVertexGpuStorage.GetGpuBuffer(); }
        [[nodiscard]] RenderHandle<GpuView>   GetStaticMeshVertexStorageShaderResourceView() const noexcept { return staticMeshVertexGpuStorage.GetShaderResourceView(); }
        [[nodiscard]] GpuFence&               GetStaticMeshVertexStorageFence() noexcept { return staticMeshVertexGpuStorage.GetStorageFence(); }
        [[nodiscard]] RenderHandle<GpuBuffer> GetVertexIndexStorageBuffer() const noexcept { return vertexIndexGpuStorage.GetGpuBuffer(); }
        [[nodiscard]] RenderHandle<GpuView>   GetVertexIndexStorageShaderResourceView() const noexcept { return vertexIndexGpuStorage.GetShaderResourceView(); }
        [[nodiscard]] GpuFence&               GetVertexIndexStorageFence() noexcept { return vertexIndexGpuStorage.GetStorageFence(); }

        [[nodiscard]] Statistics GetStatistics() const;

    private:
        LocalFrameIndex                                             currentLocalFrameIdx = 0;
        RenderContext&                                              renderContext;
        DeferredResourceManagePackage<Space<VertexSM>, MeshStorage> staticMeshVertexSpacePackage;
        GpuStorage                                                  staticMeshVertexGpuStorage;
        DeferredResourceManagePackage<Space<U32>, MeshStorage>      vertexIndexSpacePackage;
        GpuStorage                                                  vertexIndexGpuStorage;
    };
}
