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
    const float4x4 worldMat = transpose(float4x4(
        meshInstance.ToWorld[0],
        meshInstance.ToWorld[1],
        meshInstance.ToWorld[2],
        float4(0.f, 0.f, 0.f, 1.f)));
    
    bool bIsVisible = dispatchThreadId < meshLod.NumMeshlets;
    if (bIsVisible)
    {
        Meshlet meshlet = meshletStorage[meshLod.MeshletStorageOffset + dispatchThreadId];
        BoundingSphere worldBoundingSphere = TransformBoundingSphere(meshlet.BoundingVolume, worldMat);
        BoundingSphere viewBoundingSphere;
        viewBoundingSphere.Center = mul(float4(worldBoundingSphere.Center, 1.f), perFrameParams.View).xyz;
        viewBoundingSphere.Radius = worldBoundingSphere.Radius + MESHLET_CONSERVATIVE_BOUDNING_RADIUS;

        /* Per Meshlet Frustum Culling */
        bIsVisible = IntersectFrustum(
            perFrameParams.CamWorldPosInvAspectRatio.w,
            perFrameParams.ViewFrustumParams,
            viewBoundingSphere);

        /* Per Meshlet Normal Cone Culling */
        /* normalCone.xyz: Axis, normalCone.w: Cutoff */
        float4 normalCone = DecodeNormalCone(meshlet.EncodedNormalCone);
        float3 worldNormalConeAxis = normalize(mul(float4(normalCone.xyz, 0.f), worldMat)).xyz;
        float3 boundingSphereDir = worldBoundingSphere.Center - perFrameParams.CamWorldPosInvAspectRatio.xyz;
        float boundingSphereDist = length(boundingSphereDir);
        float cutOff = mad(normalCone.w, boundingSphereDist, worldBoundingSphere.Radius);
        bIsVisible &= dot(boundingSphereDir, worldNormalConeAxis) <= cutOff;
    }

    /* Compaction */
    if (bIsVisible)
    {
        const uint idx = WavePrefixCountBits(bIsVisible);
        gPayload.MeshletIndices[idx] = dispatchThreadId;
    }

    const uint numVisibleMeshlets = WaveActiveCountBits(bIsVisible);
    DispatchMesh(numVisibleMeshlets, 1, 1, gPayload);
}
