#include "ForwardMeshInstancePass.hlsli"

float4 main(VertexOutput input) : SV_Target
{
    return float4(
        float(input.MeshletIdx & 1),
        float(input.MeshletIdx & 3) / 4.f,
        float(input.MeshletIdx & 7) / 8.f,
        1.f);
}
