#include "MeshInstancePass.hlsli"
#include "Utils.hlsli"

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);

struct ZPrePassVertexOutput
{
    float4 Position : SV_Position;
};

[NumThreads(MESHLET_MAX_TRIANGLES, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint groupThreadId : SV_GroupThreadID, uint groupId : SV_GroupID,
    in payload MeshInstancePassPayload payload,
    out vertices ZPrePassVertexOutput verts[MESHLET_MAX_INDICES],
    out indices uint3 tris[MESHLET_MAX_TRIANGLES])
{
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    ConstantBuffer<SceneProxyConstants> sceneProxyConstants = ResourceDescriptorHeap[gParams.SceneProxyConstantsCbv];
    ConstantBuffer<UnifiedMeshStorageConstants> unifiedMeshStorageConstants = ResourceDescriptorHeap[gParams.UnifiedMeshStorageConstantsCbv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[sceneProxyConstants.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[sceneProxyConstants.StaticMeshStorageSrv];
    StructuredBuffer<Meshlet> meshletStorage = ResourceDescriptorHeap[unifiedMeshStorageConstants.MeshletStorageSrv];
    StructuredBuffer<uint> triangleStorage = ResourceDescriptorHeap[unifiedMeshStorageConstants.TriangleStorageSrv];
    StructuredBuffer<uint> indexStorage = ResourceDescriptorHeap[unifiedMeshStorageConstants.IndexStorageSrv];
    ByteAddressBuffer vertexStorage = ResourceDescriptorHeap[unifiedMeshStorageConstants.VertexStorageSrv];

    MeshInstance meshInstance = meshInstanceStorage[gParams.MeshInstanceIdx];
    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];
    MeshLod meshLod = mesh.LevelOfDetails[gParams.TargetLevelOfDetail];

    Meshlet meshlet = meshletStorage[meshLod.MeshletStorageOffset + payload.MeshletIndices[groupId]];
    uint numIndices = meshlet.NumIndices;
    uint numTriangles = meshlet.NumTriangles;

    SetMeshOutputCounts(numIndices, numTriangles);

    if (groupThreadId < numIndices)
    {
        const uint indexStorageIdx = meshLod.IndexStorageOffset + meshlet.IndexOffset + groupThreadId;
        const uint vertexIdx = indexStorage[indexStorageIdx];
        const uint vertexByteOffset = mad(sizeof(Vertex), vertexIdx, mesh.VertexStorageByteOffset);
        const Vertex vertex = vertexStorage.Load<Vertex>(vertexByteOffset);
        ZPrePassVertexOutput vertexOutput;
        float4x4 world = transpose(float4x4(meshInstance.ToWorld[0], meshInstance.ToWorld[1], meshInstance.ToWorld[2], float4(0.f, 0.f, 0.f, 1.f)));
        float4x4 worldViewProj = mul(world, perFrameParams.ViewProj);
        vertexOutput.Position = mul(float4(vertex.Position, 1.f), worldViewProj);
        verts[groupThreadId] = vertexOutput;
    }

    if (groupThreadId < numTriangles)
    {
        const uint triangleStorageIdx = meshLod.TriangleStorageOffset + meshlet.TriangleOffset + groupThreadId;
        const uint encodedTriangle = triangleStorage[triangleStorageIdx];
        tris[groupThreadId] = DecodeTriangle(encodedTriangle);
    }
}
