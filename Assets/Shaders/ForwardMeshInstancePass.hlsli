#ifndef _FORWARD_MESH_INSTANCE_PASS_H_
#define _FORWARD_MESH_INSTANCE_PASS_H_

#include "NewTypes.hlsli"

#define MESH_INSTANCE_PASS_AS_GROUP_SIZE 32

struct MeshInstancePassPayload
{
    uint MeshletIndices[MESH_INSTANCE_PASS_AS_GROUP_SIZE];
};

struct VertexOutput
{
    float4 Position : SV_Position;
    uint MeshletIdx : Color0;
};

#endif
