struct ComputeCullingConstants
{
    uint PerFrameDataCbv;
    uint DrawCommandStorageUav;
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
    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;
};

struct RenderableData
{
    uint TransformIdx;
    uint MaterialIdx;
    uint VertexOffset; // deprecated
    uint NumVertices; // deprecated
    uint IndexOffset;
    uint NumIndices;
};

struct DrawRenderableCommand
{
    uint PerFrameDataCbv;
    uint RenderableIndex;

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

    DrawRenderableCommand newDrawCmd;
    newDrawCmd.PerFrameDataCbv = perFrameData.PerFrameDataCbv;
    newDrawCmd.RenderableIndex = renderableIdx;

    RenderableData renderableData = renderableStorage[renderableIdx];
    newDrawCmd.VertexCountPerInstance = renderableData.NumIndices;
    newDrawCmd.InstanceCount = 1;
    newDrawCmd.StartVertexLocation = 0;
    newDrawCmd.StartInstanceLocation = 0;

    AppendStructuredBuffer<DrawRenderableCommand> drawCmdStorage = ResourceDescriptorHeap[gComputeCullingConstantsBuffer.DrawCommandStorageUav];
    drawCmdStorage.Append(newDrawCmd);
}
