struct GenDrawCommandsConstants
{
    uint PerFrameDataCbv;
    uint DrawOpaqueStaticMeshCmdBufferUav;
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

struct DrawOpaqueStaticMesh
{
    uint PerFrameDataCbv;
    uint InstancingId;
    uint MaterialIdx;
    uint VertexOffset;
    uint VertexIdxStorageOffset;
    uint TransformIdxStorageOffset;

    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

ConstantBuffer<GenDrawCommandsConstants> gGenDrawCmdConstants : register(b0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gGenDrawCmdConstants.PerFrameDataCbv];
    StructuredBuffer<InstancingData> instancingDataStorage = ResourceDescriptorHeap[perFrameData.InstancingDataStorageSrv];
    InstancingData instancingData = instancingDataStorage[DTid.x];
    if (instancingData.NumInstances > 0)
    {
        StructuredBuffer<MeshData> meshDataStorage = ResourceDescriptorHeap[perFrameData.MeshStorageSrv];
        MeshData meshData = meshDataStorage[instancingData.MeshDataIdx];

        DrawOpaqueStaticMesh newDrawCmd;
        newDrawCmd.PerFrameDataCbv = gGenDrawCmdConstants.PerFrameDataCbv;
        newDrawCmd.InstancingId = DTid.x;
        newDrawCmd.MaterialIdx = instancingData.MaterialDataIdx;
        newDrawCmd.VertexOffset = meshData.VertexOffset;
        newDrawCmd.VertexIdxStorageOffset = meshData.IndexOffset;
        newDrawCmd.TransformIdxStorageOffset = instancingData.TransformOffset;
        newDrawCmd.VertexCountPerInstance = meshData.NumIndices;
        newDrawCmd.InstanceCount = instancingData.NumInstances;

        newDrawCmd.StartVertexLocation = 0;
        newDrawCmd.StartInstanceLocation = 0;

        AppendStructuredBuffer<DrawOpaqueStaticMesh> drawOpaqueCmds = ResourceDescriptorHeap[gGenDrawCmdConstants.DrawOpaqueStaticMeshCmdBufferUav];
        drawOpaqueCmds.Append(newDrawCmd);
    }
}