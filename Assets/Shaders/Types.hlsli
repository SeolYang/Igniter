#ifndef NEW_TYPE_H
#define NEW_TYPE_H

struct PerFrameParams
{
    float4x4 View;
    float4x4 Proj;
    float4x4 ViewProj;

    uint VertexStorageSrv;
    uint IndexStorageSrv;
    uint TriangleStorageSrv;
    uint MeshletStorageSrv;

    uint LightStorageSrv;
    uint MaterialStorageSrv;
    uint StaticMeshStorageSrv;
    uint MeshInstanceStorageSrv;

    /* (x,y,z): cam pos, w: inv aspect ratio */
    float4 CamWorldPosInvAspectRatio;
    /* x: cos(fovy/2), y: sin(fovy/2), z: near, w: far */
    float4 ViewFrustumParams;
};

struct ScreenParams
{
    uint Width;
    uint Height;
};

struct Light
{
    uint Type;
    float Intensity;
    float FalloffRadius;
    float3 Color;
    float3 WorldPos;
    float3 Forward;
};

struct Material
{
    uint DiffuseTexSrv;
    uint DiffuseTexSampler;
};

/* Deprecated! */
struct VertexSM
{
    float3 Position;
    float3 Normal;
    float2 TexCoords;
};

struct Vertex
{
    float3 Position;
    uint QuantizedNormal;                    /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
    uint QuantizedTangent;                   /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
    uint QuantizedBitangent;                 /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
    vector<float16_t, 2> QuantizedTexCoords; /* (F32, F32) -> (F16, F16) = HLSL(float16_t, float16_t); https://github.com/zeux/meshoptimizer/blob/master/src/quantization.cpp*/
    uint ColorRGBA8;                         /* R8G8B8A8_UINT */
};

struct BoundingSphere
{
    float3 Center;
    float Radius;
};

#define MESHLET_MAX_INDICES 64
#define MESHLET_MAX_TRIANGLES 64

struct Meshlet
{
    uint IndexOffset;
    uint NumIndices;

    uint TriangleOffset;
    uint NumTriangles;

    BoundingSphere BoundingVolume;

    float3 NormalConeAxis;
    float NormalConeCutoff;
};

struct MeshLod
{
    uint MeshletStorageOffset;
    uint NumMeshlets;
    uint IndexStorageOffset;
    uint TriangleStorageOffset;
};

#define MAX_MESH_LEVEL_OF_DETAILS 8

struct Mesh
{
    uint VertexStorageByteOffset;
    uint NumLevelOfDetails;
    MeshLod LevelOfDetails[MAX_MESH_LEVEL_OF_DETAILS];
    BoundingSphere BoundingVolume;
    uint2 Padding;
};

#define MESH_TYPE_STATIC 0
#define MESH_TYPE_SKELETAL 1

struct MeshInstance
{
    uint MeshType;
    uint MeshProxyIdx;
    uint MaterialProxyIdx;

    float4 ToWorld[3];

    uint Padding;
};

struct DispatchMeshInstanceParams
{
    uint MeshInstanceIdx;
    uint TargetLevelOfDetail;
    uint PerFrameParamsCbv;
    uint ScreenParamsCbv;
};

struct DispatchMeshInstance
{
    DispatchMeshInstanceParams Params;
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
};

#endif
