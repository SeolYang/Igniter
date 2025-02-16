#include "ForwardMeshInstancePass.hlsli"
#define MESHLET_CONSERVATIVE_BOUDNING_RADIUS 0.1f;

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);
groupshared MeshInstancePassPayload gPayload;

[NumThreads(MESH_INSTANCE_PASS_AS_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[perFrameParams.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[perFrameParams.StaticMeshStorageSrv];
    StructuredBuffer<Meshlet> meshletStorage = ResourceDescriptorHeap[perFrameParams.MeshletStorageSrv];
    MeshInstance meshInstance = meshInstanceStorage[gParams.MeshInstanceIdx];
    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];
    MeshLod meshLod = mesh.LevelOfDetails[gParams.TargetLevelOfDetail];

    bool bIsVisible = dispatchThreadId < meshLod.NumMeshlets;
    if (bIsVisible)
    {
        const float4x4 worldMat = transpose(float4x4(
            meshInstance.ToWorld[0],
            meshInstance.ToWorld[1],
            meshInstance.ToWorld[2],
            float4(0.f, 0.f, 0.f, 1.f)));
        Meshlet meshlet = meshletStorage[meshLod.MeshletStorageOffset + dispatchThreadId];

        const BoundingSphere worldBoundingSphere =
            TransformBoundingSphere(meshlet.BoundingVolume, worldMat);
        BoundingSphere viewBoundingSphere;
        viewBoundingSphere.Center = mul(float4(worldBoundingSphere.Center, 1.f), perFrameParams.View).xyz;
        viewBoundingSphere.Radius = worldBoundingSphere.Radius + MESHLET_CONSERVATIVE_BOUDNING_RADIUS;
        bIsVisible = IntersectFrustum(
            perFrameParams.CamWorldPosInvAspectRatio.w,
            perFrameParams.ViewFrustumParams,
            viewBoundingSphere);
        
        const uint idx = WavePrefixCountBits(bIsVisible);
        gPayload.MeshletIndices[idx] = dispatchThreadId;
    }

    const uint numVisibleMeshlets = WaveActiveCountBits(bIsVisible);
    DispatchMesh(numVisibleMeshlets, 1, 1, gPayload);
}
