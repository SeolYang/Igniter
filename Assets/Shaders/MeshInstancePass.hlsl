#include "Types.hlsli"
#include "Utils.hlsli"
#include "Constants.hlsli"

struct MeshInstancePassParams
{
    uint PerFrameParamsCbv;
    uint MeshInstanceIndicesBufferSrv;
    uint NumMeshInstances;

    uint OpaqueMeshInstanceDispatchBufferUav;
    uint TransparentMeshInstanceDispatchBufferUav;
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
    StructuredBuffer<uint> meshInstanceIndicesBuffer = ResourceDescriptorHeap[gMeshInstanceParams.MeshInstanceIndicesBufferSrv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[perFrameParams.MeshInstanceStorageSrv];
    StructuredBuffer<Mesh> staticMeshStorage = ResourceDescriptorHeap[perFrameParams.StaticMeshStorageSrv];

    const uint meshInstanceIdx = meshInstanceIndicesBuffer[DTid.x];
    const MeshInstance meshInstance = meshInstanceStorage[meshInstanceIdx];

    Mesh mesh = staticMeshStorage[meshInstance.MeshProxyIdx];

    const float4x4 worldMat = transpose(float4x4(
        meshInstance.ToWorld[0],
        meshInstance.ToWorld[1],
        meshInstance.ToWorld[2],
        float4(0.f, 0.f, 0.f, 1.f)));
    const float4x4 worldViewProj = mul(worldMat, perFrameParams.ViewProj);

    const BoundingSphere worldBoundingSphere =
        TransformBoundingSphere(mesh.BoundingVolume, worldMat);
    BoundingSphere viewBoundingSphere;
    viewBoundingSphere.Center = mul(float4(worldBoundingSphere.Center, 1.f), perFrameParams.View).xyz;
    viewBoundingSphere.Radius = worldBoundingSphere.Radius;
    bool bIsVisible = IntersectFrustum(
        perFrameParams.CamWorldPosInvAspectRatio.w,
        perFrameParams.ViewFrustumParams,
        viewBoundingSphere);
    if (!bIsVisible)
    {
        return;
    }

    // 1. Local Space Bounding Sphere를 사용해 Local Space AABB를 생성한다  
    // 2. Local Space AABB를 WVP 변환을 통해 Clip Space로 변환 한다.  
    // 3. Clip Space AABB의 각 점을 w로 나누어 Perspective Division을 수행해 NDC로 변환 한다.  
    // 4. NDC AABB에서 x, y 의 min/max 값들을 구한다  
    // 5. 이렇게 구해진 min/max_x/y를 통해 x/y축 변의 길이를 구한다.  
    // 6. x_len * y_len을 4로 나누면 NDC에서 Bounding Sphere가 차지하는 영역을 근사화 한 것 이다. (NDC에서 Screen Space로 사상되는 영역의 너비가 4이기 때문(NDC.x/y in \[-1, 1]))
    // 7. 이 비율은 Bounding Sphere를 근사화한 AABB가 화면에서 차지하는 픽셀의 비율을 근사화 한 것 이다.  
    // 8. 별도의 Clipping(clamp)는 하지 않는다. 그 이유는 일부가 화면 밖에서 나가더라도,  
    // 여전히 메시는 시점에서 가까운 위치에 있을 수 있기 때문. 예를들어 화면 반을 차지하는 메시가 있다고 하더라도  
    // 동일한 거리에서 화면 전체를 차지하는 동일한 메시와 LOD는 일치해야 할 것이다.
    // 다만 최종 비율은 [0, 1]로 clamping 해주어야 한다(saturate)
    // 9. 만약 MeshInstance가 LOD Screen Coverage Threshold를 사용한다면 MeshInstance에 제공된 값을 사용하여 특정 Threshold를 넘는 기준으로
    // LOD를 선택한다. (bOverrideLodScreenCoverageThreshold == true) 이 때, LevelOfDetails가 낮을 수록(더 자세한 모델), 높은 Threshold를
    // 설정해야 한다. (0.9 == 전체 픽셀의 90% 차지 시)
    // 10. 그렇지 않다면 Scale로 전체 LOD를 Linear 하게 증가 시킨다. (Lerp(1.f - computedCoverage, 0, NumLevelOfDetails))
    /* xmin, ymin, xmax, ymax */
    float4 aabbNdc = float4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (uint idx = 0; idx < 8; ++idx)
    {
        float4 aabbCornerLocal = float4(mesh.BoundingVolume.Center, 1.f) + (mesh.BoundingVolume.Radius * kAabbCornerOffsets[idx]);
        float4 aabbCornerNdc = mul(aabbCornerLocal, worldViewProj);
        // Perspective Division: Clip -> NDC
        aabbCornerNdc /= aabbCornerNdc.w;

        aabbNdc.x = min(aabbCornerNdc.x, aabbNdc.x);
        aabbNdc.y = min(aabbCornerNdc.y, aabbNdc.y);
        aabbNdc.z = max(aabbCornerNdc.x, aabbNdc.z);
        aabbNdc.w = max(aabbCornerNdc.y, aabbNdc.w);
    }
    const float screenCoverage = saturate(((aabbNdc.z - aabbNdc.x) * (aabbNdc.w - aabbNdc.y)) * 0.25f);
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
    
    MeshLod meshLod = mesh.LevelOfDetails[targetLevelOfDetail];
    DispatchMeshInstance newDispatchMeshInstance;
    newDispatchMeshInstance.Params.MeshInstanceIdx = meshInstanceIdx;
    newDispatchMeshInstance.Params.TargetLevelOfDetail = targetLevelOfDetail;
    newDispatchMeshInstance.Params.PerFrameParamsCbv = gMeshInstanceParams.PerFrameParamsCbv;
    newDispatchMeshInstance.ThreadGroupCountX = (meshLod.NumMeshlets + 31) / 32;
    newDispatchMeshInstance.ThreadGroupCountY = 1;
    newDispatchMeshInstance.ThreadGroupCountZ = 1;

    // if opaque pass? transparent pass? alpha-tested pass?
    AppendStructuredBuffer<DispatchMeshInstance> opaqueMeshInstanceDispatchBuffer = ResourceDescriptorHeap[gMeshInstanceParams.OpaqueMeshInstanceDispatchBufferUav];
    opaqueMeshInstanceDispatchBuffer.Append(newDispatchMeshInstance);
}
