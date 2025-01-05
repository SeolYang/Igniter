struct ComputeCullingConstants
{
    uint PerFrameDataCbv;
    uint DrawOpaqueStaticMeshCmdBufferUav;
};

struct PerFrameData
{
    float4x4 ViewProj;
    float4 CameraPos;
    float4 CameraForward;

    uint StaticMeshVertexStorageSrv;
    uint VertexIndexStorageSrv;

    uint TransformStorageSrv;
    uint MaterialStorageSrv;

    uint StaticMeshStorageSrv;

    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;
};

struct StaticMeshData
{
    uint TransformIdx;
    uint MaterialIdx;
    uint VertexOffset;
    uint NumVertices;
    uint IndexOffset;
    uint NumIndices;
};

struct RenderableData
{
    uint Type;
    uint DataIdx;
};

struct DrawOpaqueStaticMesh
{
    uint PerFrameDataCbv;
    uint StaticMeshDataIdx;

    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

ConstantBuffer<ComputeCullingConstants> gComputeCullingConstantsBuffer : register(b0);

[numthreads(16, 1, 1)]
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
    
    if (renderableData.Type == 0) // 하드코딩된 Static Mesh Type Enumerator
    {
        DrawOpaqueStaticMesh newDrawCmd;
        newDrawCmd.PerFrameDataCbv = perFrameData.PerFrameDataCbv;
        newDrawCmd.StaticMeshDataIdx = renderableData.DataIdx;

        StructuredBuffer<StaticMeshData> staticMeshStorage = ResourceDescriptorHeap[perFrameData.StaticMeshStorageSrv];
        StaticMeshData staticMeshData = staticMeshStorage[renderableData.DataIdx];
        newDrawCmd.VertexCountPerInstance = staticMeshData.NumIndices;
        newDrawCmd.InstanceCount = 1;
        newDrawCmd.StartVertexLocation = 0;
        newDrawCmd.StartInstanceLocation = 0;

        AppendStructuredBuffer<DrawOpaqueStaticMesh> drawCmdBuffer = ResourceDescriptorHeap[gComputeCullingConstantsBuffer.DrawOpaqueStaticMeshCmdBufferUav];
        drawCmdBuffer.Append(newDrawCmd);
    }
}
