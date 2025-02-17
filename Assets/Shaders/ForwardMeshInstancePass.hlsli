#ifndef _FORWARD_MESH_INSTANCE_PASS_H_
#define _FORWARD_MESH_INSTANCE_PASS_H_

#include "Types.hlsli"
#include "Utils.hlsli"
#include "Constants.hlsli"

#define MESH_INSTANCE_PASS_AS_GROUP_SIZE 32

struct MeshInstancePassPayload
{
    uint MeshletIndices[MESH_INSTANCE_PASS_AS_GROUP_SIZE];
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Normal : Normal;
    float2 TexCoord0 : TexCoord0;
    float3 WorldPosition : WorldPosition;
};

#endif
