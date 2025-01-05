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

struct Vertex
{
    float3 aPos;
    float3 aNormal;
    float2 aUv;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint StaticMeshDataIdx;
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

struct VertexShaderOutput
{
    float4 aPos : SV_POSITION;
    float2 aUv : TEXCOORD;
};

ConstantBuffer<PerDrawData> perDrawData : register(b0);

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[perDrawData.PerFrameDataCbv];
    StructuredBuffer<Vertex> vertexStorage = ResourceDescriptorHeap[perFrameData.StaticMeshVertexStorageSrv];
    StructuredBuffer<uint> vertexIndexStorage = ResourceDescriptorHeap[perFrameData.VertexIndexStorageSrv];
    StructuredBuffer<StaticMeshData> staticMeshStorage = ResourceDescriptorHeap[perFrameData.StaticMeshStorageSrv];
    StructuredBuffer<float4x4> transformStorage = ResourceDescriptorHeap[perFrameData.TransformStorageSrv];

    StaticMeshData staticMeshData = staticMeshStorage[perDrawData.StaticMeshDataIdx];
    float4x4 worldMat = transformStorage[staticMeshData.TransformIdx];
    float4x4 worldViewProj = mul(worldMat, perFrameData.ViewProj);

    uint vertexOffset = staticMeshData.VertexOffset;
    uint vertexIndexOffset = staticMeshData.IndexOffset;
    Vertex vertex = vertexStorage[vertexOffset + vertexIndexStorage[vertexIndexOffset + vertexID]];

    VertexShaderOutput output;
    output.aPos = mul(float4(vertex.aPos.xyz, 1.f), worldViewProj);
    output.aUv  = vertex.aUv;
    return output;
}
