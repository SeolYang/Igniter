#include "Common.hlsl"

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint MaterialIdx;
    uint VertexOffset;
    uint VertexIdxOffset;
    uint IndirectTransformOffset;
};
ConstantBuffer<PerDrawData> perDrawData : register(b0);

struct VertexShaderOutput
{
    float4 aPos : SV_POSITION;
};

VertexShaderOutput main(uint vertexID : SV_VertexID, uint instanceId : SV_InstanceID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[perDrawData.PerFrameDataCbv];
    StructuredBuffer<VertexSM> vertexStorage = ResourceDescriptorHeap[perFrameData.StaticMeshVertexStorageSrv];
    StructuredBuffer<uint> vertexIndexStorage = ResourceDescriptorHeap[perFrameData.VertexIndexStorageSrv];
    StructuredBuffer<uint> indirectTransformStorage = ResourceDescriptorHeap[perFrameData.IndirectTransformStorageSrv];
    StructuredBuffer<TransformData> transformStorage = ResourceDescriptorHeap[perFrameData.TransformStorageSrv];

    uint transformIdx = indirectTransformStorage[perDrawData.IndirectTransformOffset + instanceId];
    TransformData transformData = transformStorage[transformIdx];
    float4x4 worldMat = transpose(float4x4(transformData.Cols[0], transformData.Cols[1], transformData.Cols[2], float4(0.f, 0.f, 0.f, 1.f)));
    float4x4 worldViewProj = mul(worldMat, perFrameData.ToViewProj);

    VertexSM vertex = vertexStorage[perDrawData.VertexOffset + vertexIndexStorage[perDrawData.VertexIdxOffset + vertexID]];
    VertexShaderOutput output;
    output.aPos = mul(float4(vertex.Position.xyz, 1.f), worldViewProj);
    return output;
}
