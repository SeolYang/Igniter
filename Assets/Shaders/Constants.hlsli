#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define FLT_MAX 3.402823466e+38F
#define PI 3.141592f

/*
* Mesh Constants
*/
#define MAX_MESH_LOD 8
#define RENDERABLE_TYPE_STATIC_MESH 0

/*
* Light Clustering Constants
*/
#define MAX_LIGHTS 32768
#define NUM_DEPTH_BINS 4096
#define MAX_DEPTH_BIN_IDX (NUM_DEPTH_BINS - 1)
#define MAX_DEPTH_BIN_IDX_F32 float(MAX_DEPTH_BIN_IDX)
#define LIGHT_SIZE_EPSILON 0.0001f
#define TILE_SIZE_UINT 16
#define TILE_SIZE_F32 float(TILE_SIZE_UINT)
#define INV_TILE_SIZE 1.f/TILE_SIZE_F32
#define NUM_U32_PER_TILE (MAX_LIGHTS / 32)

#define NUM_AABB_VERTICES 8
static const float3 kAabbCornerOffsets[NUM_AABB_VERTICES] =
{
    float3(-1.f, -1.f, -1.f),
    float3(-1.f, 1.f, -1.f),
    float3(1.f, 1.f, -1.f),
    float3(1.f, -1.f, -1.f),
    float3(1.f, 1.f, 1.f),
    float3(1.f, -1.f, 1.f),
    float3(-1.f, 1.f, 1.f),
    float3(-1.f, -1.f, 1.f)
};

#define NUM_AABB_TRIANGLES 12
static const uint3 kAabbTriangles[NUM_AABB_TRIANGLES] =
{
    uint3(0, 1, 2),
    uint3(0, 2, 3),
    uint3(3, 2, 4),
    uint3(3, 4, 5),
    uint3(5, 4, 6),
    uint3(5, 6, 7),
    uint3(7, 6, 0),
    uint3(0, 6, 1),
    uint3(1, 6, 2),
    uint3(2, 6, 4),
    uint3(0, 7, 3),
    uint3(3, 7, 5)
};

#endif
