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

struct Vertex
{
    float3 aPos;
    float3 aNormal;
    float2 aUv;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint RenderableIdx;
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
    StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
    StructuredBuffer<float4x4> transformStorage = ResourceDescriptorHeap[perFrameData.TransformStorageSrv];

    RenderableData renderableData = renderableStorage[perDrawData.RenderableIdx];
    float4x4 worldMat = transformStorage[renderableData.TransformIdx];
    float4x4 worldViewProj = mul(worldMat, perFrameData.ViewProj);

    uint vertexOffset = renderableData.VertexOffset;
    uint vertexIndexOffset = renderableData.IndexOffset;
    Vertex vertex = vertexStorage[vertexOffset + vertexIndexStorage[vertexIndexOffset + vertexID]];

    VertexShaderOutput output;
    output.aPos = mul(float4(vertex.aPos.xyz, 1.f), worldViewProj);
    output.aUv  = vertex.aUv;
    return output;
}
