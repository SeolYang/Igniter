#include "ForwardMeshInstancePass.hlsli"
#define MESHLET_CONSERVATIVE_BOUDNING_RADIUS 0.1f;

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);
groupshared MeshInstancePassPayload gPayload;

[NumThreads(MESH_INSTANCE_PASS_AS_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    ConstantBuffer<DepthPyramidParams> depthPyramidParams = ResourceDescriptorHeap[gParams.DepthPyramidParamsCbv];
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

        /* Per Meshlet Hi-Z Occlusion Culling */
        float4 aabbScreenUv;
        bIsVisible &= ProjectSphere(
            viewBoundingSphere.Center, viewBoundingSphere.Radius,
            perFrameParams.ViewFrustumParams.z,
            perFrameParams.Proj._m00, perFrameParams.Proj._m11,
            aabbScreenUv);

        if (bIsVisible)
        {
            Texture2D depthPyramid = ResourceDescriptorHeap[depthPyramidParams.DepthPyramidSrv];
            float boundingSphereWidth = (aabbScreenUv.z - aabbScreenUv.x) * depthPyramidParams.DepthPyramidWidth;
            float boundingSphereHeight = (aabbScreenUv.w - aabbScreenUv.y) * depthPyramidParams.DepthPyramidHeight;
            uint targetDepthPyramidMipLevel = min(floor(log2(max(boundingSphereWidth, boundingSphereHeight))), depthPyramidParams.NumDepthPyramidMips - 1);
            SamplerState depthPyramidSampler = ResourceDescriptorHeap[depthPyramidParams.DepthPyramidSampler];
            float2 depthPyramidUv = (aabbScreenUv.xy + aabbScreenUv.zw) * 0.5f;
            float depth = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidUv, targetDepthPyramidMipLevel).r;
            float zSphere = max(perFrameParams.ViewFrustumParams.z, viewBoundingSphere.Center.z - viewBoundingSphere.Radius);
            const float kCalibrationCoefficient = max(1.f, 10.f / viewBoundingSphere.Radius);
            float depthSphere = (perFrameParams.ViewFrustumParams.z / zSphere) * kCalibrationCoefficient;
            if (depth > depthSphere)
            {
                bIsVisible = false;
                /* Depth Discontinuity 대처 */
                while (targetDepthPyramidMipLevel > 1)
                {
                    targetDepthPyramidMipLevel -= 1;
                    float2 depthPyramidSamplePoint0 = aabbScreenUv.xy;
                    float2 depthPyramidSamplePoint1 = aabbScreenUv.xw;
                    float2 depthPyramidSamplePoint2 = aabbScreenUv.zy;
                    float2 depthPyramidSamplePoint3 = aabbScreenUv.zw;
                    float depth0 = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidSamplePoint0, targetDepthPyramidMipLevel).r;
                    float depth1 = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidSamplePoint1, targetDepthPyramidMipLevel).r;
                    float depth2 = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidSamplePoint2, targetDepthPyramidMipLevel).r;
                    float depth3 = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidSamplePoint3, targetDepthPyramidMipLevel).r;
                    if ((depth0 <= depthSphere) || (depth1 <= depthSphere) || (depth2 <= depthSphere) || (depth3 <= depthSphere))
                    {
                        bIsVisible = true;
                        break;
                    }
                }
            }
        }
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
