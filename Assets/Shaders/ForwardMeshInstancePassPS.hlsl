#include "ForwardMeshInstancePass.hlsli"

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);

[earlydepthstencil]
float4 main(VertexOutput input) : SV_Target
{
    return float4(
    clamp(float(input.MeshletIndex & 7) / 8.f, 0.1f, 1.f),
    clamp(float(input.MeshletIndex & 15) / 16.f, 0.1f, 1.f),
    clamp(float(input.MeshletIndex & 31) / 32.f, 0.2f, 1.f),
    1.f);
    
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[perFrameParams.MeshInstanceStorageSrv];
    StructuredBuffer<Material> materialStorage = ResourceDescriptorHeap[perFrameParams.MaterialStorageSrv];
    MeshInstance meshInstance = meshInstanceStorage[gParams.MeshInstanceIdx];
    Material material = materialStorage[meshInstance.MaterialProxyIdx];

    Texture2D texture = ResourceDescriptorHeap[material.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[material.DiffuseTexSampler];
    const float3 diffuse = texture.Sample(samplerState, input.TexCoord0).rgb;
    
    return float4(
        diffuse,
        1.f);
}
