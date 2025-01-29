#include "Common.hlsl"

//#define MAX_LIGHTS (65536/4)
#define MAX_LIGHTS 8192
#define NUM_DEPTH_BINS 32
#define MAX_DEPTH_BIN_IDX 31
#define MAX_DEPTH_BIN_IDX_F32 31.f
#define LIGHT_SIZE_EPSILON 0.0001f
#define INV_TILE_SIZE 1.f/8.f
#define NUM_U32_PER_TILE (MAX_LIGHTS / 32)

struct LightClusteringConstants
{
    uint PerFrameDataCbv;
    uint LightClusteringParamsCbv;
};

struct LightClusteringParams
{
    float4x4 ToProj;
    float ViewportWidth;
    float ViewportHeight;
    
    uint NumLights;
    
    uint LightStorageBufferSrv;
    uint LightIdxListBufferSrv;
    uint TileDwordsBufferUav;
    uint DepthBinsBufferUav;
};

ConstantBuffer<LightClusteringConstants> gConstants;

static const float4 kAabbCornerOffsets[8] =
{
    float4(-1.f, -1.f, -1.f, 0.f),
    float4(-1.f, 1.f, -1.f, 0.f),
    float4(1.f, 1.f, -1.f, 0.f),
    float4(1.f, -1.f, -1.f, 0.f),
    float4(-1.f, 1.f, 1.f, 0.f),
    float4(1.f, 1.f, 1.f, 0.f),
    float4(1.f, -1.f, 1.f, 0.f),
    float4(-1.f, -1.f, 1.f, 0.f)
};

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<LightClusteringParams> params = ResourceDescriptorHeap[gConstants.LightClusteringParamsCbv];
    
    RWStructuredBuffer<uint2> depthBins = ResourceDescriptorHeap[params.DepthBinsBufferUav];
    if (DTid.x < NUM_DEPTH_BINS)
    {
        depthBins[DTid.x] = uint2(0xFFFFFFFF, 0);
    }
    DeviceMemoryBarrierWithGroupSync();
    
    if (DTid.x >= params.NumLights)
    {
        return;
    }
    
    RWStructuredBuffer<uint> tileDwords = ResourceDescriptorHeap[params.TileDwordsBufferUav];
    
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gConstants.PerFrameDataCbv];
    float4 s_camWorldPos = float4(perFrameData.CamPosInvAspectRatio.x, perFrameData.CamPosInvAspectRatio.y, perFrameData.CamPosInvAspectRatio.z, 1.f);
    float s_nearPlane = perFrameData.ViewFrustumParams.z;
    float s_farPlane = perFrameData.ViewFrustumParams.w;
    float s_invCamPlaneDist = 1.f / (s_farPlane - s_nearPlane);
    
    StructuredBuffer<uint> lightIdxList = ResourceDescriptorHeap[params.LightIdxListBufferSrv];
    StructuredBuffer<Light> lightStorage = ResourceDescriptorHeap[params.LightStorageBufferSrv];
    uint v_lightIdx = lightIdxList[DTid.x];
    Light v_light = lightStorage[v_lightIdx];
    
    float4 v_lightPosView = mul(float4(v_light.WorldPos.x, v_light.WorldPos.y, v_light.WorldPos.z, 1.f), perFrameData.ToView);
    
    // 타일 정보만 구하면 되기 때문에 aabbScreen = (min.x, min.y, max.x, max.y)
    float4 v_aabbScreen = float4(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (uint idx = 0; idx < 8; ++idx)
    {
        float4 v_aabbCornerView = v_lightPosView + (v_light.Radius * kAabbCornerOffsets[idx]);
        //// *Corner Case!*: AABB를 z축에 따라 clipping
        v_aabbCornerView.z = clamp(v_aabbCornerView.z, s_nearPlane, s_farPlane);
        
        float4 v_aabbCornerScreen = mul(v_aabbCornerView, params.ToProj);
        // Perspective Division => NDC
        v_aabbCornerScreen /= v_aabbCornerScreen.w;
        //// 이미 절두체 밖으로 나간 점을 Near/Far Plane에 project 하면 오히려 잘못된 결과를 만들어냄
        //v_aabbCornerScreen.z = clamp(v_aabbCornerScreen.z, 0.f, 1.f);
        
        // Viewport Transformation NDC x,y in [-1, 1] -> SS x,y in [0, Viewport.Width), [0, Viewport.Height)
        v_aabbCornerScreen.x = (v_aabbCornerScreen.x + 1.f) * 0.5f * (params.ViewportWidth - 1.f);
        v_aabbCornerScreen.y = (-v_aabbCornerScreen.y + 1.f) * 0.5f * (params.ViewportHeight - 1.f);

        v_aabbScreen.x = min(v_aabbCornerScreen.x, v_aabbScreen.x);
        v_aabbScreen.y = min(v_aabbCornerScreen.y, v_aabbScreen.y);
        v_aabbScreen.z = max(v_aabbCornerScreen.x, v_aabbScreen.z);
        v_aabbScreen.w = max(v_aabbCornerScreen.y, v_aabbScreen.w);
    }
    
    // Depth Bin Culling 조건
    const float v_nearPointZView = v_lightPosView.z - v_light.Radius;
    const float v_farPointZView = v_lightPosView.z + v_light.Radius;
    const bool bDepthBinCullCond0 = v_farPointZView < s_nearPlane || v_nearPointZView > s_farPlane;
    const bool bDepthBinCullCond1 = v_lightPosView.z < s_nearPlane || v_lightPosView.z > s_farPlane;
    const bool bCulledByDepthBin = bDepthBinCullCond0 && bDepthBinCullCond1;
    
    // Tile Culling 조건
    const bool bTileCullCond0 = v_aabbScreen.x > params.ViewportWidth || v_aabbScreen.z < 0.f;
    const bool bTileCullCond1 = v_aabbScreen.y > params.ViewportHeight || v_aabbScreen.w < 0.f;
    const bool bTileCullCond2 = (v_aabbScreen.z - v_aabbScreen.x) < LIGHT_SIZE_EPSILON || (v_aabbScreen.w - v_aabbScreen.y) < LIGHT_SIZE_EPSILON;
    const bool bCulledByTile = bTileCullCond0 || bTileCullCond1 || bTileCullCond2;
    
    if (bCulledByDepthBin || bCulledByTile)
    {
        return;
    }

    // Depth Bin 채우기
    int v_minDepthBinIdx = int(32.f * ((v_nearPointZView - s_nearPlane) * s_invCamPlaneDist));
    int v_maxDepthBinIdx = int(32.f * ((v_farPointZView - s_nearPlane) * s_invCamPlaneDist));
    
    v_minDepthBinIdx = max(0, v_minDepthBinIdx);
    v_maxDepthBinIdx = min(MAX_DEPTH_BIN_IDX, v_maxDepthBinIdx);
        
    for (uint depthBinIdx = uint(v_minDepthBinIdx); depthBinIdx <= uint(v_maxDepthBinIdx); ++depthBinIdx)
    {
        InterlockedMin(depthBins[depthBinIdx].x, DTid.x);
        InterlockedMax(depthBins[depthBinIdx].y, DTid.x);
    }
    
    // Tile 채우기
    v_aabbScreen.x = clamp(v_aabbScreen.x, 0.f, params.ViewportWidth);
    v_aabbScreen.y = clamp(v_aabbScreen.y, 0.f, params.ViewportHeight);
    v_aabbScreen.z = clamp(v_aabbScreen.z, 0.f, params.ViewportWidth);
    v_aabbScreen.w = clamp(v_aabbScreen.w, 0.f, params.ViewportHeight);
    
    v_aabbScreen *= INV_TILE_SIZE;
    const uint v_firstTileX = uint(v_aabbScreen.x);
    const uint v_firstTileY = uint(v_aabbScreen.y);
    const uint v_lastTileX = uint(v_aabbScreen.z);
    const uint v_lastTileY = uint(v_aabbScreen.w);
    
    const uint s_numTileX = uint(params.ViewportWidth * INV_TILE_SIZE);
    for (uint v_tileX = v_firstTileX; v_tileX < v_lastTileX; ++v_tileX)
    {
        for (uint v_tileY = v_firstTileY; v_tileY < v_lastTileY; ++v_tileY)
        {
            const uint v_tileDwordOffset = ((s_numTileX * v_tileY) + v_tileX) * NUM_U32_PER_TILE;
            const uint v_lightDwordOffset = v_tileDwordOffset + (DTid.x / 32);
            const uint v_lightDword = (uint(1) << (DTid.x % 32));
            InterlockedOr(tileDwords[v_lightDwordOffset], v_lightDword);
        }
    }
}