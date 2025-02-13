#include "NewTypes.hlsli"

struct MeshInstancePassParams
{
    uint PerFrameParamsCbv;
    uint MeshInstanceIndicesBufferSrv;
    uint NumMeshInstances;

    uint OpaqueMeshInstanceDispatchBufferUav;
    uint TransparentMeshInstanceDispatchBufferUav;

    uint ManualLod;
};

ConstantBuffer<MeshInstancePassParams> gMeshInstanceParams : register(b0);

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gMeshInstanceParams.NumMeshInstances)
    {
        return;
    }
    
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gMeshInstanceParams.PerFrameParamsCbv];
    StructuredBuffer<uint> meshInstanceIndicesBuffer = ResourceDescriptorHeap[gMeshInstanceParams.MeshInstanceIndicesBufferSrv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[perFrameParams.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[perFrameParams.StaticMeshStorageSrv];

    const uint meshInstanceIdx = meshInstanceIndicesBuffer[DTid.x];
    const MeshInstance meshInstance = meshInstanceStorage[meshInstanceIdx];

    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];
    uint targetLevelOfDetail = min(gMeshInstanceParams.ManualLod, mesh.NumLevelOfDetails);
    MeshLod meshLod = mesh.LevelOfDetails[targetLevelOfDetail];

    DispatchMeshInstance newDispatchMeshInstance;
    newDispatchMeshInstance.MeshInstanceIdx = meshInstanceIdx;
    newDispatchMeshInstance.TargetLevelOfDetail = targetLevelOfDetail;
    newDispatchMeshInstance.PerFrameParamsCbv = gMeshInstanceParams.PerFrameParamsCbv;
    newDispatchMeshInstance.ScreenParamsCbv = 0xFFFFFFFF;
    newDispatchMeshInstance.ThreadGroupCountX = meshLod.NumMeshlets;
    newDispatchMeshInstance.ThreadGroupCountY = 1;
    newDispatchMeshInstance.ThreadGroupCountZ = 1;

    // if opaque pass? transparent pass? alpha-tested pass?
    AppendStructuredBuffer<DispatchMeshInstance> opaqueMeshInstanceDispatchBuffer = ResourceDescriptorHeap[gMeshInstanceParams.OpaqueMeshInstanceDispatchBufferUav];
    opaqueMeshInstanceDispatchBuffer.Append(newDispatchMeshInstance);
}