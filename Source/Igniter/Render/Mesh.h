#pragma once
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    /* 실제 존재하지 않는 타입. Unified Mesh Storage로 부터 Lookup시 그에 맞는 GpuStorage::Allocation 객체를 반환 */
    struct Meshlet;
    struct MeshIndex;
    struct MeshTriangle;
    struct MeshVertex;

    inline U32 EncodeTriangleU32(const U8 p0, const U8 p1, const U8 p2)
    {
        return (U32)p0 | ((U32)p1 << 8) | ((U32)p2 << 16);
    }

    struct MeshLod
    {
    public:
        Handle<Meshlet> MeshletStorageAlloc{};
        Handle<MeshIndex> IndexStorageAlloc{};
        Handle<MeshTriangle> TriangleStorageAlloc{};
    };

    enum class EMeshType : U32
    {
        Static   = 0,
        Skeletal = 1
    };

    struct Mesh
    {
    public:
        constexpr static U8 kNumVertexPerTriangle = 3;
        constexpr static U8 kMaxMeshLevelOfDetails = 8;

    public:
        Handle<MeshVertex> VertexStorageAlloc{};
        U8 NumLevelOfDetails = 0;
        MeshLod LevelOfDetails[kMaxMeshLevelOfDetails];
        AABB BoundingBox{};
    };

    /* Meshlet의 경우 단순 데이터이기 때문에, 별도의 CPU/GPU 간 데이터 레이아웃의 차이가 없다 */
    struct Meshlet
    {
    public:
        /*
         * https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/
         * https://www.youtube.com/watch?v=EtX7WnFhxtQ  (AW2)
         */
        constexpr static Size kMaxVertices = 64;
        constexpr static Size kMaxTriangles = 64;

    public:
        /* MeshLod::IndexStorageAlloc 내에서의 IndexOffset */
        U32 IndexOffset = 0;
        U32 NumIndices = 0;
        /* MeshLod::TriangleStorageAlloc 내에서의 TriangleOffset */
        U32 TriangleOffset = 0;
        U32 NumTriangles = 0;

        BoundingSphere BoundingVolume{};

        Vector3 NormalConeAxis{};
        float NormalConeCutoff = 0.f;
        // U8 QuantizedNormalConeAxis[3];
        // U8 QuantizedNormalConeCutoff;
    };

    struct GpuMeshLod
    {
    public:
        U32 MeshletStorageOffset = 0;
        U32 NumMeshlets = 0;
        U32 IndexStorageOffset = 0;
        U32 TriangleStorageOffset = 0;
    };

    struct GpuMesh
    {
    public:
        U32 VertexStorageByteOffset = 0;
        U32 NumLevelOfDetails = 0;
        GpuMeshLod LevelOfDetails[Mesh::kMaxMeshLevelOfDetails];

        BoundingSphere MeshBoundingSphere{};

        U64 Padding = 0xFFFFFFFFFFFFFFFFUi64;
    };

    struct GpuMeshInstance
    {
    public:
        EMeshType MeshType = EMeshType::Static;
        U32 MeshProxyIdx = 0;
        U32 MaterialProxyIdx = 0;

        Vector4 ToWorld[3];

        U32 Padding = 0xFFFFFFFF;
    };
} // namespace ig
