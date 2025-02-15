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

static const float4 kAabbCornerOffsets[8] =
{
    float4(-1.f, -1.f, -1.f, 0.f),
    float4(-1.f, 1.f, -1.f, 0.f),
    float4(1.f, 1.f, -1.f, 0.f),
    float4(1.f, -1.f, -1.f, 0.f),
    float4(-1.f, 1.f, 1.f, 0.f),
    float4(1.f, 1.f, 1.f, 0.f),
    float4(1.f, -1.f, 1.f, 0.f),
    float4(-1.f, -1.f, 1.f, 0.f)
};

#endif
