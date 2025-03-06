#pragma once
#include "Igniter/Render/Common.h"
#include "Igniter/Render/Mesh.h"
#include "Igniter/Render/GpuStorage.h"

namespace ig
{
    struct MeshVertexAllocation
    {
        GpuStorage::Allocation Alloc{};
        Size NumVertices{0};
        Size SizeOfVertex{0}; // or stride
    };

    class RenderContext;
    class GpuStorage;

    class UnifiedMeshStorage
    {
    private:
        struct GpuStorageConstants
        {
            U32 VertexStorageSrv = IG_NUMERIC_MAX_OF(VertexStorageSrv);
            U32 IndexStorageSrv = IG_NUMERIC_MAX_OF(IndexStorageSrv);
            U32 TriangleStorageSrv = IG_NUMERIC_MAX_OF(TriangleStorageSrv);
            U32 MeshletStorageSrv = IG_NUMERIC_MAX_OF(MeshletStorageSrv);
        };

    public:
        explicit UnifiedMeshStorage(RenderContext& renderContext);
        UnifiedMeshStorage(const UnifiedMeshStorage&) = delete;
        UnifiedMeshStorage(UnifiedMeshStorage&&) noexcept = delete;
        ~UnifiedMeshStorage();

        UnifiedMeshStorage& operator=(const UnifiedMeshStorage&) = delete;
        UnifiedMeshStorage& operator=(UnifiedMeshStorage&&) noexcept = delete;

        template <typename VertexType>
        Handle<MeshVertex> AllocateVertices(const Size numVertices)
        {
            static_assert(sizeof(VertexType) > 0);
            static_assert(sizeof(VertexType) % 4 == 0);
            return AllocateVertices(numVertices, sizeof(VertexType) / 4);
        }

        Handle<MeshIndex> AllocateIndices(const Size numIndices);
        Handle<MeshTriangle> AllocateTriangles(const Size numTriangles);
        Handle<Meshlet> AllocateMeshlets(const Size numMeshlets);

        void Deallocate(const Handle<MeshVertex> handle);
        void Deallocate(const Handle<MeshIndex> handle);
        void Deallocate(const Handle<MeshTriangle> handle);
        void Deallocate(const Handle<Meshlet> handle);

        [[nodiscard]] const MeshVertexAllocation* Lookup(const Handle<MeshVertex> handle) const noexcept;
        [[nodiscard]] const GpuStorage::Allocation* Lookup(const Handle<MeshIndex> handle) const noexcept;
        [[nodiscard]] const GpuStorage::Allocation* Lookup(const Handle<MeshTriangle> handle) const noexcept;
        [[nodiscard]] const GpuStorage::Allocation* Lookup(const Handle<Meshlet> handle) const noexcept;

        void PreRender(const LocalFrameIndex localFrameIdx);

        [[nodiscard]] Handle<GpuBuffer> GetVertexStorageBuffer() const noexcept { return vertexStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetVertexStorageSrv() const noexcept { return vertexStorage.GetSrv(); }
        [[nodiscard]] Handle<GpuBuffer> GetIndexStorageBuffer() const noexcept { return indexStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetIndexStorageSrv() const noexcept { return indexStorage.GetSrv(); }
        [[nodiscard]] Handle<GpuBuffer> GetTriangleStorageBuffer() const noexcept { return triangleStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetTriangleStorageSrv() const noexcept { return triangleStorage.GetSrv(); }
        [[nodiscard]] Handle<GpuBuffer> GetMeshletStorageBuffer() const noexcept { return meshletStorage.GetGpuBuffer(); }
        [[nodiscard]] Handle<GpuView> GetMeshletStorageSrv() const noexcept { return meshletStorage.GetSrv(); }

        [[nodiscard]] Size GetNumAllocatedVerticesHint() const noexcept { return numAllocVertices; }
        [[nodiscard]] Size GetNumAllocatedIndicesHint() const noexcept { return numAllocIndices; }
        [[nodiscard]] Size GetNumAllocatedTrianglesHint() const noexcept { return numAllocTriangles; }
        [[nodiscard]] Size GetNumAllocatedMeshletsHint() const noexcept { return numAllocMeshlets; }

        [[nodiscard]] Handle<GpuView> GetStorageConstantsCbv() const noexcept { return gpuStorageConstantsCbv; }

    private:
        Handle<MeshVertex> AllocateVertices(const Size numVertices, const Size numDwordsPerVertex);

    private:
        RenderContext* renderContext = nullptr;

        LocalFrameIndex currentLocalFrameIdx = 0;
        /*
         * 정점 데이터를 저장, 4바이트 단위 할당, 셰이더에서 정점의 타입에 따라 원하는 타입으로 해석 가능
         * ByteAddressBuffer를 사용하여 여러가지 타입의 정점을 한번에 관리 할 수 있도록 가정
         */
        GpuStorage vertexStorage;
        Size numAllocVertices = 0;
        DeferredResourceManagePackage<MeshVertexAllocation> vertexDeferredManagePackage;
        /*
         * 정점의 인덱스를 저장, 4바이트 단위 할당, 셰이더에서 정점 저장소로 부터 데이터를 읽기 위한, 정점 인덱스(Byte Offset이 아님)
         * vertexStorage.Load(VertexStorageByteOffset + (Index * VertexStride))
         * vertexStorage.Load(mad(Index, VertexStride, VertexStorageByteOffset))
         * Level Of Indirection: 1
         */
        GpuStorage indexStorage;
        Size numAllocIndices = 0;
        DeferredResourceManagePackage<GpuStorage::Allocation> indexDeferredManagedPackage;
        /*
         * Meshlet 내부의 로컬 인덱스를 저장, 4바이트 단위 할당 (3*U8, 1*U8(Padding))
         * triangle_p0_local_idx_offset = triangleStorage.Load(TriangleOffset + TriangleIdx) & 0x8;
         * triangle_p0_idx_offset = IndexStorageOffset + triangle_p0_local_idx_offset;
         * triangle_p0_idx = indexStorage.Load(triangle_p0_idx_offset);
         * triangle_p0_vert = vertexStorage.Load(mad(triangle_p0_idx, VertexStride, VertexStorageByteOffset));
         * Level Of Indirection: 2
         */
        GpuStorage triangleStorage;
        Size numAllocTriangles = 0;
        DeferredResourceManagePackage<GpuStorage::Allocation> triangleDeferredManagePackage;
        /*
         * Meshlet 데이터 저장, 16바이트 단위 할당
         * IndexOffset: LOD 또는 메시 로컬 인덱스 오프셋
         * NumIndices: Meshlet의 인덱스 수
         * TriangleOffset: LOD 또는 메시 로컬 삼각형 오프셋
         * NumTriangles: Meshlet의 삼각형 수
         */
        GpuStorage meshletStorage;
        Size numAllocMeshlets = 0;
        DeferredResourceManagePackage<GpuStorage::Allocation> meshletDeferredManagePackage;

        GpuStorageConstants gpuStorageConstants;
        Handle<GpuBuffer> gpuStorageConstantsBuffer;
        Handle<GpuView> gpuStorageConstantsCbv;
    };
} // namespace ig
