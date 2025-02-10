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
        Handle<Meshlet> MeshletStorageAlloc;
        Handle<MeshIndex> IndexStorageAlloc;
        Handle<MeshTriangle> TriangleStorageAlloc;
    };

    constexpr U8 kNumVertexPerTriangle = 3;
    constexpr U8 kMaxMeshLevelOfDetails = 8;
    struct Mesh
    {
      public:
        Handle<MeshVertex> VertexStorageAlloc;
        AABB BoundingBox;

        MeshLod LevelOfDetails[kMaxMeshLevelOfDetails];
    };

    /* Meshlet의 경우 단순 데이터이기 때문에, 별도의 CPU/GPU 간 데이터 레이아웃의 차이가 없다 */
    struct Meshlet
    {
      public:
        /* MeshLod::IndexStorageAlloc 내에서의 IndexOffset */
        U32 IndexOffset;
        U32 NumIndices;
        /* MeshLod::TriangleStorageAlloc 내에서의 TriangleOffset */
        U32 TriangleOffset;
        U32 NumTriangles;

        BoundingSphere ClusterBoundingSphere;

        Vector3 NormalConeAxis;
        float NormalConeCutoff;
        // U8 QuantizedNormalConeAxis[3];
        // U8 QuantizedNormalConeCutoff;
    };

    struct GpuMeshLod
    {
      public:
        U32 MeshletStorageOffset;
        U32 NumMeshlets;
        U32 IndexStorageOffset;
        U32 TriangleStorageOffset;
    };

    struct GpuMesh
    {
      public:
        U32 VertexStorageByteOffset;
        BoundingSphere MeshBoundingSphere;

        GpuMeshLod LevelOfDetails[kMaxMeshLevelOfDetails];
    };
} // namespace ig
