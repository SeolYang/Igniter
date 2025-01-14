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

struct Vertex
{
    float3 aPos;
    float3 aNormal;
    float2 aUv;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint InstancingId;
    uint MaterialIdx;
    uint VertexOffset;
    uint VertexIdxStorageOffset;
    uint TransformIdxStorageOffset;
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

struct MeshData
{
    uint VertexOffset;
    uint NumVertices;
    uint IndexOffset;
    uint NumIndices;

    float3 BsCentroid;
    float BsRadius;
};

struct VertexShaderOutput
{
    float4 aPos : SV_POSITION;
    float2 aUv : TEXCOORD;
};

struct TransformData
{
    float4 Cols[3];
};

ConstantBuffer<PerDrawData> perDrawData : register(b0);

VertexShaderOutput main(uint vertexID : SV_VertexID, uint instanceId : SV_InstanceID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[perDrawData.PerFrameDataCbv];
    StructuredBuffer<Vertex> vertexStorage = ResourceDescriptorHeap[perFrameData.StaticMeshVertexStorageSrv];
    StructuredBuffer<uint> vertexIndexStorage = ResourceDescriptorHeap[perFrameData.VertexIndexStorageSrv];
    StructuredBuffer<uint> transformIdxStorage = ResourceDescriptorHeap[perFrameData.TransformIdxStorageSrv];
    StructuredBuffer<TransformData> transformStorage = ResourceDescriptorHeap[perFrameData.TransformStorageSrv];

    uint transformIdx =  transformIdxStorage[perDrawData.TransformIdxStorageOffset + instanceId];
    TransformData transformData = transformStorage[transformIdx];
    float4x4 worldMat = transpose(float4x4(transformData.Cols[0], transformData.Cols[1], transformData.Cols[2], float4(0.f, 0.f, 0.f, 1.f)));
    float4x4 worldViewProj = mul(worldMat, perFrameData.ViewProj);

    Vertex vertex = vertexStorage[perDrawData.VertexOffset + vertexIndexStorage[perDrawData.VertexIdxStorageOffset + vertexID]];
    VertexShaderOutput output;
    output.aPos = mul(float4(vertex.aPos.xyz, 1.f), worldViewProj);
    output.aUv  = vertex.aUv;
    return output;
}
