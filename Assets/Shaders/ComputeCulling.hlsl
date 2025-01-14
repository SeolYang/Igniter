struct ComputeCullingConstants
{
    uint PerFrameDataCbv;
};

struct PerFrameData
{
    float4x4 ViewProj;
    float4 CameraPos;

    uint StaticMeshVertexStorageSrv;
    uint VertexIndexStorageSrv;

    uint TransformStorageSrv;
    uint MaterialStorageSrv;
    uint MeshStorageSrv;

    uint InstancingDataStorageSrv;
    uint InstancingDataStorageUav;

    uint TransformIdxStorageSrv;
    uint TransformIdxStorageUav;

    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;

    uint3 Padding;
};

struct MeshData
{
    uint VertexOffset;
    uint NumVertices;
    uint IndexOffset;
    uint NumIndices;

    float3 BsCentroid;
    float BsRadius;
};

struct InstancingData
{
    uint MaterialDataIdx;
    uint MeshDataIdx;
    uint InstanceId;
    uint TransformOffset;
    uint MaxNumInstances;
    uint NumInstances;
};

struct RenderableData
{
    uint Type;
    uint DataIdx;
    uint TransformIdx;
};


#define RENDERABLE_TYPE_STATIC_MESH 0

ConstantBuffer<ComputeCullingConstants> gComputeCullingConstantsBuffer : register(b0);

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gComputeCullingConstantsBuffer.PerFrameDataCbv];

    if (DTid.x >= perFrameData.NumMaxRenderables)
    {
        return;
    }

    StructuredBuffer<uint> renderableIndices = ResourceDescriptorHeap[perFrameData.RenderableIndicesBufferSrv];
    uint renderableIdx = renderableIndices[DTid.x];
    StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
    RenderableData renderableData = renderableStorage[renderableIdx];
    
    // 무조건 통과한다고 일단 가정.
    if (renderableData.Type == RENDERABLE_TYPE_STATIC_MESH)
    {
        RWStructuredBuffer<InstancingData> instancingDataStorage = ResourceDescriptorHeap[perFrameData.InstancingDataStorageUav];
        RWStructuredBuffer<uint> transformIdxStorage = ResourceDescriptorHeap[perFrameData.TransformIdxStorageUav];
        uint transformIdx = 0;
        InterlockedAdd(instancingDataStorage[renderableData.DataIdx].NumInstances, 1, transformIdx);
        transformIdxStorage[instancingDataStorage[renderableData.DataIdx].TransformOffset + transformIdx] = renderableData.TransformIdx;
    }
}
