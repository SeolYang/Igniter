#include "Types.hlsli"
#include "Utils.hlsli"

struct MeshInstancePassParams
{
    uint PerFrameParamsCbv;
    uint UnifiedMeshStorageConstantsCbv;
    uint SceneProxyConstantsCbv;
    uint NumMeshInstances;

    uint OpaqueMeshInstanceDispatchBufferUav;
    uint TransparentMeshInstanceDispatchBufferUav;

    uint DepthPyramidParamsCbv;
};

ConstantBuffer<MeshInstancePassParams> gMeshInstanceParams : register(b0);

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= gMeshInstanceParams.NumMeshInstances)
    {
        return;
    }

    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gMeshInstanceParams.PerFrameParamsCbv];
    ConstantBuffer<SceneProxyConstants> sceneProxyConstants = ResourceDescriptorHeap[gMeshInstanceParams.SceneProxyConstantsCbv];
    ConstantBuffer<DepthPyramidParams> depthPyramidParams = ResourceDescriptorHeap[gMeshInstanceParams.DepthPyramidParamsCbv];
    StructuredBuffer<uint> meshInstanceIndicesBuffer = ResourceDescriptorHeap[sceneProxyConstants.MeshInstanceIndicesBufferSrv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[sceneProxyConstants.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[sceneProxyConstants.StaticMeshStorageSrv];

    const uint meshInstanceIdx = meshInstanceIndicesBuffer[DTid.x];
    const MeshInstance meshInstance = meshInstanceStorage[meshInstanceIdx];

    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];

    const float4x4 worldMat = transpose(float4x4(
        meshInstance.ToWorld[0],
        meshInstance.ToWorld[1],
        meshInstance.ToWorld[2],
        float4(0.f, 0.f, 0.f, 1.f)));

    const BoundingSphere worldBoundingSphere =
        TransformBoundingSphere(mesh.BoundingVolume, worldMat);
    BoundingSphere viewBoundingSphere;
    viewBoundingSphere.Center = mul(float4(worldBoundingSphere.Center, 1.f), perFrameParams.View).xyz;
    viewBoundingSphere.Radius = worldBoundingSphere.Radius;

    /* Per Mesh Instance Frustum Culling */
    bool bIsVisible = IntersectFrustum(
        perFrameParams.CamWorldPosInvAspectRatio.w,
        perFrameParams.ViewFrustumParams,
        viewBoundingSphere);
    if (!bIsVisible)
    {
        return;
    }
    /**************************************/

    /* Per Mesh Instance Hi-Z Occlusion Culling */
    const static float kConservativeBoundingSphereRadiusFactor = 0.5f;
    Texture2D depthPyramid = ResourceDescriptorHeap[depthPyramidParams.DepthPyramidSrv];
    float4 aabbScreenUv;
    if (!ProjectSphere(
        // 실제 Radius보다 조금 더 보수적으로?
        viewBoundingSphere.Center, viewBoundingSphere.Radius * kConservativeBoundingSphereRadiusFactor,
        perFrameParams.ViewFrustumParams.z,
        perFrameParams.Proj._m00, perFrameParams.Proj._m11,
        aabbScreenUv))
    {
        return;
    }

    float boundingSphereWidth = (aabbScreenUv.z - aabbScreenUv.x) * depthPyramidParams.DepthPyramidWidth;
    float boundingSphereHeight = (aabbScreenUv.w - aabbScreenUv.y) * depthPyramidParams.DepthPyramidHeight;
    uint targetDepthPyramidMipLevel = min(floor(log2(max(boundingSphereWidth, boundingSphereHeight))), depthPyramidParams.NumDepthPyramidMips - 1);
    SamplerState depthPyramidSampler = ResourceDescriptorHeap[depthPyramidParams.DepthPyramidSampler];
    float2 depthPyramidUv = (aabbScreenUv.xy + aabbScreenUv.zw) * 0.5f;
    float depth = depthPyramid.SampleLevel(depthPyramidSampler, depthPyramidUv, targetDepthPyramidMipLevel).r;
    float zSphere = max(perFrameParams.ViewFrustumParams.z, viewBoundingSphere.Center.z - viewBoundingSphere.Radius);

    /* 좀 더 나은 대체 값은 없을 까? */
    const float kCalibrationCoefficient = max(1.f, 10.f / viewBoundingSphere.Radius);
    float depthSphere = (perFrameParams.ViewFrustumParams.z / zSphere) * kCalibrationCoefficient;
    /* Reversed-Z!; Depth가 1.f에 가까울 수록 가깝고, 0.f에 가까울 수록 멀다 */
    if (depth > depthSphere)
    {
        /* Depth Discontinuity 대처 */
        bool bLeastVisible = false;
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
                bLeastVisible = true;
                break;
            }
        }
        if (!bLeastVisible)
        {
            return;
        }
    }
    /**************************************/

    /* @todo 현재 계산된 AABB는 Bounding Sphere의 Radius를 보수적으로 설정하였기 때문에, 그에 맞춰서 coverage 값을 보정해주어야 한다.  */
    const float screenCoverage = saturate((aabbScreenUv.z - aabbScreenUv.x) * (aabbScreenUv.w - aabbScreenUv.y));
    uint targetLevelOfDetail;
    if (mesh.bOverrideLodScreenCoverageThreshold)
    {
        for (uint lod = 0; lod < mesh.NumLevelOfDetails; ++lod)
        {
            if (screenCoverage >= mesh.LodScreenCoverageThresholds[lod])
            {
                targetLevelOfDetail = lod;
                break;
            }
        }
    }
    else
    {
        targetLevelOfDetail = MapScreenCoverageToLodAuto(screenCoverage, mesh.NumLevelOfDetails);
    }
    targetLevelOfDetail = min(targetLevelOfDetail, mesh.NumLevelOfDetails - 1);
    /**************************************/

    MeshLod meshLod = mesh.LevelOfDetails[targetLevelOfDetail];
    DispatchMeshInstance newDispatchMeshInstance;
    newDispatchMeshInstance.Params.MeshInstanceIdx = meshInstanceIdx;
    newDispatchMeshInstance.Params.TargetLevelOfDetail = targetLevelOfDetail;
    newDispatchMeshInstance.Params.PerFrameParamsCbv = gMeshInstanceParams.PerFrameParamsCbv;
    newDispatchMeshInstance.Params.UnifiedMeshStorageConstantsCbv = gMeshInstanceParams.UnifiedMeshStorageConstantsCbv;
    newDispatchMeshInstance.Params.SceneProxyConstantsCbv = gMeshInstanceParams.SceneProxyConstantsCbv;
    newDispatchMeshInstance.Params.DepthPyramidParamsCbv = gMeshInstanceParams.DepthPyramidParamsCbv;
    newDispatchMeshInstance.ThreadGroupCountX = (meshLod.NumMeshlets + 31) / 32;
    newDispatchMeshInstance.ThreadGroupCountY = 1;
    newDispatchMeshInstance.ThreadGroupCountZ = 1;

    // if opaque pass? transparent pass? alpha-tested pass?
    AppendStructuredBuffer<DispatchMeshInstance> opaqueMeshInstanceDispatchBuffer = ResourceDescriptorHeap[gMeshInstanceParams.OpaqueMeshInstanceDispatchBufferUav];
    opaqueMeshInstanceDispatchBuffer.Append(newDispatchMeshInstance);
}
