#include "ForwardMeshInstancePass.hlsli"

float4 main(VertexOutput input) : SV_Target
{
    return float4(
        clamp(float(input.MeshletIdx & 7) / 8.f, 0.1f, 1.f),
        clamp(float(input.MeshletIdx & 15) / 16.f, 0.1f, 1.f),
        clamp(float(input.MeshletIdx & 31) / 32.f, 0.2f, 1.f),
        1.f);
}
