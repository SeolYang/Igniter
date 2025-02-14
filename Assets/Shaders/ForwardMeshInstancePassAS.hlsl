#include "ForwardMeshInstancePass.hlsli"

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);
groupshared MeshInstancePassPayload gPayload;

[NumThreads(MESH_INSTANCE_PASS_AS_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[perFrameParams.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[perFrameParams.StaticMeshStorageSrv];
    MeshInstance meshInstance = meshInstanceStorage[gParams.MeshInstanceIdx];
    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];
    MeshLod meshLod = mesh.LevelOfDetails[gParams.TargetLevelOfDetail];

    bool bVisible = false;
    if (dispatchThreadId < meshLod.NumMeshlets)
    {
        bVisible = true;
    }

    if (bVisible)
    {
        const uint idx = WavePrefixCountBits(bVisible);
        gPayload.MeshletIndices[idx] = dispatchThreadId;
    }

    const uint numVisibleMeshlets = WaveActiveCountBits(bVisible);
    DispatchMesh(numVisibleMeshlets, 1, 1, gPayload);
}
