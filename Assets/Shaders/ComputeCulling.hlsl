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
    uint MeshStorageSrv;

    uint StaticMeshStorageSrv;

    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;
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

struct StaticMeshData
{
    uint MaterialIdx;
    uint MeshIdx;
};

struct RenderableData
{
    uint Type;
    uint DataIdx;
    uint TransformIdx;
};

struct DrawOpaqueStaticMesh
{
    uint PerFrameDataCbv;
    uint RenderableDataIdx;

    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

#define RENDERABLE_TYPE_STATIC_MESH 0

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
    
    if (renderableData.Type == RENDERABLE_TYPE_STATIC_MESH)
    {
        DrawOpaqueStaticMesh newDrawCmd;
        newDrawCmd.PerFrameDataCbv = perFrameData.PerFrameDataCbv;
        newDrawCmd.RenderableDataIdx = renderableIdx;

        StructuredBuffer<StaticMeshData> staticMeshStorage = ResourceDescriptorHeap[perFrameData.StaticMeshStorageSrv];
        StaticMeshData staticMeshData = staticMeshStorage[renderableData.DataIdx];

        StructuredBuffer<MeshData> meshStorage = ResourceDescriptorHeap[perFrameData.MeshStorageSrv];
        MeshData meshData = meshStorage[staticMeshData.MeshIdx];

        newDrawCmd.VertexCountPerInstance = meshData.NumIndices;
        newDrawCmd.InstanceCount = 1;
        newDrawCmd.StartVertexLocation = 0;
        newDrawCmd.StartInstanceLocation = 0;

        AppendStructuredBuffer<DrawOpaqueStaticMesh> drawCmdBuffer = ResourceDescriptorHeap[gComputeCullingConstantsBuffer.DrawOpaqueStaticMeshCmdBufferUav];
        drawCmdBuffer.Append(newDrawCmd);
    }
}
