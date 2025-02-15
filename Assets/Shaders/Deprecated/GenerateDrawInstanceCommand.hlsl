#include "Common.hlsl"

struct GenerateInstanceDrawCommandsConstants
{
    uint PerFrameDataCbv;
    uint CullingDataBufferSrv;
    uint DrawInstanceBufferUav;
};

ConstantBuffer<GenerateInstanceDrawCommandsConstants> gConstants : register(b0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gConstants.PerFrameDataCbv];
    StructuredBuffer<InstancingData> instancingDataStorage = ResourceDescriptorHeap[perFrameData.InstancingDataStorageSrv];
    StructuredBuffer<CullingData> cullingDataBuffer = ResourceDescriptorHeap[gConstants.CullingDataBufferSrv];
    if (DTid.x >= cullingDataBuffer[0].NumVisibleLodInstances)
    {
        return;
    }

    InstancingData instancingData = instancingDataStorage[DTid.x];

    uint lodIndirectTransformOffset = instancingData.IndirectTransformOffset;
    for (uint lod = 0; lod < MAX_MESH_LOD; ++lod)
    {
        if (instancingData.NumVisibleLodInstances[lod] > 0)
        {
            AppendStructuredBuffer<DrawInstance> drawInstanceBuffer = ResourceDescriptorHeap[gConstants.DrawInstanceBufferUav];

            StructuredBuffer<Mesh> meshStorage = ResourceDescriptorHeap[perFrameData.MeshStorageSrv];
            Mesh mesh = meshStorage[instancingData.MeshIdx];

            DrawInstance newDrawInstance;
            newDrawInstance.PerFrameDataCbv = gConstants.PerFrameDataCbv;
            newDrawInstance.MaterialIdx = instancingData.MaterialIdx;
            newDrawInstance.VertexOffset = mesh.VertexOffset;
            newDrawInstance.VertexIdxOffset = mesh.Lods[lod].IndexOffset;
            newDrawInstance.IndirectTransformOffset = lodIndirectTransformOffset;

            newDrawInstance.VertexCountPerInstance = mesh.Lods[lod].NumIndices;
            newDrawInstance.InstanceCount = instancingData.NumVisibleLodInstances[lod];
            newDrawInstance.StartVertexLocation = 0;
            newDrawInstance.StartInstanceLocation = 0;

            drawInstanceBuffer.Append(newDrawInstance);

            lodIndirectTransformOffset += instancingData.NumVisibleLodInstances[lod];
        }
    }
}
