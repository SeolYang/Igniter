#include "Types.hlsli"
#include "Constants.hlsli"
#include "Utils.hlsli"

struct LightClusteringConstants
{
    uint PerFrameParamsCbv;
    uint SceneProxyConstantsCbv;
    uint LightIdxListSrv;
    uint TileBitfieldsUav;
    uint DepthBinsUav;

    uint NumLights;
};

ConstantBuffer<LightClusteringConstants> gConstants;

[numthreads(32, 1, 1)]
void main(uint lightIdxListIdx : SV_DispatchThreadID)
{
    if (lightIdxListIdx >= gConstants.NumLights)
    {
        return;
    }

    RWStructuredBuffer<uint> tileBitfields = ResourceDescriptorHeap[gConstants.TileBitfieldsUav];

    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gConstants.PerFrameParamsCbv];
    ConstantBuffer<SceneProxyConstants> sceneProxyConstants = ResourceDescriptorHeap[gConstants.SceneProxyConstantsCbv];
    float s_nearPlane = perFrameParams.ViewFrustumParams.z;
    float s_farPlane = perFrameParams.ViewFrustumParams.w;
    float s_invCamPlaneDist = 1.f / (s_farPlane - s_nearPlane);

    StructuredBuffer<uint> lightIdxList = ResourceDescriptorHeap[gConstants.LightIdxListSrv];
    StructuredBuffer<Light> lightStorage = ResourceDescriptorHeap[sceneProxyConstants.LightStorageSrv];
    uint v_lightIdx = lightIdxList[lightIdxListIdx];
    Light v_light = lightStorage[v_lightIdx];

    float4 v_lightPosView = mul(float4(v_light.WorldPos.x, v_light.WorldPos.y, v_light.WorldPos.z, 1.f), perFrameParams.View);

    // Depth Bin Culling 조건
    const float v_nearPointZView = v_lightPosView.z - v_light.FalloffRadius;
    const float v_farPointZView = v_lightPosView.z + v_light.FalloffRadius;
    const bool bDepthBinCullCond0 = v_farPointZView < s_nearPlane || v_nearPointZView > s_farPlane;
    const bool bDepthBinCullCond1 = v_lightPosView.z < s_nearPlane || v_lightPosView.z > s_farPlane;
    const bool bCulledByDepthBin = bDepthBinCullCond0 && bDepthBinCullCond1;

    // 타일 정보만 구하면 되기 때문에 aabbScreen = (min.x, min.y, max.x, max.y)
    float4 v_aabbScreen = float4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (uint idx = 0; idx < 8; ++idx)
    {
        float4 v_aabbCornerView = float4(mad(v_light.FalloffRadius, kAabbCornerOffsets[idx], v_lightPosView.xyz), 1.f);
        //// *Corner Case!*: AABB를 z축에 따라 clipping
        v_aabbCornerView.z = clamp(v_aabbCornerView.z, s_nearPlane, s_farPlane);

        float4 v_aabbCornerScreen = mul(v_aabbCornerView, perFrameParams.Proj);
        // Perspective Division => NDC
        v_aabbCornerScreen /= v_aabbCornerScreen.w;
        //// 이미 절두체 밖으로 나간 점을 Near/Far Plane에 project 하면 오히려 잘못된 결과를 만들어냄
        //v_aabbCornerScreen.z = clamp(v_aabbCornerScreen.z, 0.f, 1.f);

        // Viewport Transformation NDC x,y in [-1, 1] -> SS x,y in [0, Viewport.Width), [0, Viewport.Height)
        v_aabbCornerScreen.x = (v_aabbCornerScreen.x + 1.f) * 0.5f * (perFrameParams.ViewportWidth - 1.f);
        v_aabbCornerScreen.y = (-v_aabbCornerScreen.y + 1.f) * 0.5f * (perFrameParams.ViewportHeight - 1.f);

        v_aabbScreen.x = min(v_aabbCornerScreen.x, v_aabbScreen.x);
        v_aabbScreen.y = min(v_aabbCornerScreen.y, v_aabbScreen.y);
        v_aabbScreen.z = max(v_aabbCornerScreen.x, v_aabbScreen.z);
        v_aabbScreen.w = max(v_aabbCornerScreen.y, v_aabbScreen.w);
    }

    // Depth Bin 채우기
    RWStructuredBuffer<uint2> depthBins = ResourceDescriptorHeap[gConstants.DepthBinsUav];
    if (!bCulledByDepthBin)
    {
        int v_minDepthBinIdx = int(MAX_DEPTH_BIN_IDX_F32 * ((v_nearPointZView - s_nearPlane) * s_invCamPlaneDist));
        int v_maxDepthBinIdx = int(MAX_DEPTH_BIN_IDX_F32 * ((v_farPointZView - s_nearPlane) * s_invCamPlaneDist));
        v_minDepthBinIdx = clamp(v_minDepthBinIdx, 0, MAX_DEPTH_BIN_IDX);
        v_maxDepthBinIdx = clamp(v_maxDepthBinIdx, 0, MAX_DEPTH_BIN_IDX);

        const int s_minDepthBinIdx = WaveActiveMin(v_minDepthBinIdx);
        const int s_maxDepthBinIdx = WaveActiveMax(v_maxDepthBinIdx);
        for (int s_depthBinIdx = s_minDepthBinIdx; s_depthBinIdx <= s_maxDepthBinIdx; ++s_depthBinIdx)
        {
            if (s_depthBinIdx >= v_minDepthBinIdx && s_depthBinIdx <= v_maxDepthBinIdx)
            {
                const uint s_minLightIdxListIdx = WaveActiveMin(lightIdxListIdx);
                const uint s_maxLightIdxListIdx = WaveActiveMax(lightIdxListIdx);
                if (WaveIsFirstLane())
                {
                    InterlockedMin(depthBins[s_depthBinIdx].x, s_minLightIdxListIdx);
                    InterlockedMax(depthBins[s_depthBinIdx].y, s_maxLightIdxListIdx);
                }
            }
        }
    }

    // Tile Culling 조건
    const bool bTileCullCond0 = v_aabbScreen.x > perFrameParams.ViewportWidth || v_aabbScreen.z < 0.f;
    const bool bTileCullCond1 = v_aabbScreen.y > perFrameParams.ViewportHeight || v_aabbScreen.w < 0.f;
    const bool bTileCullCond2 = (v_aabbScreen.z - v_aabbScreen.x) < LIGHT_SIZE_EPSILON || (v_aabbScreen.w - v_aabbScreen.y) < LIGHT_SIZE_EPSILON;
    const bool bCulledByTile = bTileCullCond0 || bTileCullCond1 || bTileCullCond2;
    if (!bCulledByTile && (v_farPointZView >= s_nearPlane))
    {
        // Tile 채우기
        v_aabbScreen.x = clamp(v_aabbScreen.x, 0.f, perFrameParams.ViewportWidth);
        v_aabbScreen.y = clamp(v_aabbScreen.y, 0.f, perFrameParams.ViewportHeight);
        v_aabbScreen.z = clamp(v_aabbScreen.z, 0.f, perFrameParams.ViewportWidth);
        v_aabbScreen.w = clamp(v_aabbScreen.w, 0.f, perFrameParams.ViewportHeight);

        v_aabbScreen *= INV_TILE_SIZE;
        const uint v_firstTileX = uint(v_aabbScreen.x);
        const uint v_firstTileY = uint(v_aabbScreen.y);
        const uint v_lastTileX = uint(v_aabbScreen.z);
        const uint v_lastTileY = uint(v_aabbScreen.w);
        const uint s_numTileX = uint(perFrameParams.ViewportWidth * INV_TILE_SIZE);

        // 그룹내 스레드는 항상 최악의 경우를 상정한다. 최악의 경우
        // v_tileY = 0 ~ Max, v_tileX = 0~Max를 모든 스레드가 실행한다
        // 이걸 최소화 할려면? 같은 v_tileY과 v_tileX를 가지는 thread를 모아서 한번에 처리하자
        // 그러면 InterlockedOr의 비율이 1/NumThreads로 줄어들지 않을까?
        const uint mergedFirstTileX = WaveActiveMin(v_firstTileX);
        const uint mergedFirstTileY = WaveActiveMin(v_firstTileY);
        const uint mergedLastTileX = WaveActiveMax(v_lastTileX);
        const uint mergedLastTileY = WaveActiveMax(v_lastTileY);
        const uint lightOffset = lightIdxListIdx / 32;
        for (uint tileY = mergedFirstTileY; tileY <= mergedLastTileY; ++tileY)
        {
            const uint tileYOffset = (s_numTileX * tileY) * NUM_U32_PER_TILE;
            for (uint tileX = mergedFirstTileX; tileX <= mergedLastTileX; ++tileX)
            {
                const uint tileDwordOffset = tileYOffset + tileX * NUM_U32_PER_TILE;
                const uint lightDwordOffset = tileDwordOffset + lightOffset;
                const uint mask = WaveActiveBallot(
                    (tileX >= v_firstTileX && tileX <= v_lastTileX) &&
                    (tileY >= v_firstTileY && tileY <= v_lastTileY)).x;

                if (WaveIsFirstLane())
                {
                    InterlockedOr(tileBitfields[lightDwordOffset], mask);
                }
            }
        }
    }
}
