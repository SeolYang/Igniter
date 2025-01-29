#include "Common.hlsl"

#define MAX_LIGHTS 8192
#define NUM_U32_PER_TILE (MAX_LIGHTS / 32)
#define INV_TILE_SIZE 1.f/8.f
#define NUM_DEPTH_BINS 32

struct DebugLightClustersConstants
{
    uint PerFrameDataCbv;
    uint ViewportWidth;
    uint ViewportHeight;
    
    uint NumLights;

    uint TileDwordsBufferSrv;
    
    uint InputTexSrv;
    uint OutputTexUav;
};

ConstantBuffer<DebugLightClustersConstants> gConstants : register(b0);

const static float3 ReallyLowDensityTileColor = float3(0.f, 0.0f, 0.6f);
const static float3 LowDensityTileColor = float3(0.4f, 0.8f, 0.2f);
const static float3 HighDensityTileColor = float3(1.f, 1.f, 0.f);
const static float3 ReallyHighDensityTileColor = float3(1.f, 0.f, 0.f);
const static float3 MidDensityTileColor = (LowDensityTileColor + HighDensityTileColor) * 0.5f;

// 8x8 = 64, 해당 그룹 내부의 모든 스레드들은 하나의 타일을 공유하게 된다.
// 이 경우 해당 타일에 대해서는 SGPR로 최적화 할 수 있다.
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID)
{
    if (DTid.x >= uint(gConstants.ViewportWidth - 1.f) || DTid.y >= uint(gConstants.ViewportHeight - 1.f))
    {
        return;
    }
    
    Texture2D inputTex = ResourceDescriptorHeap[gConstants.InputTexSrv];
    RWTexture2D<float4> outputTex = ResourceDescriptorHeap[gConstants.OutputTexUav];
    StructuredBuffer<uint> tileDwords = ResourceDescriptorHeap[gConstants.TileDwordsBufferSrv];
    
    const uint2 tileIdx = DTid.xy / 8;
    const uint tileStride = uint(gConstants.ViewportWidth * INV_TILE_SIZE);
    const uint tileDwordOffset = ((tileStride * tileIdx.y) + tileIdx.x) * NUM_U32_PER_TILE;
    uint numLightsInTile = 0;
    for (uint lightListIdx = 0; lightListIdx < gConstants.NumLights; ++lightListIdx)
    {
        const uint lightDwordOffset = lightListIdx / 32;
        const uint lightDwordBitOffset = lightListIdx % 32;
        if (tileDwords[tileDwordOffset + lightDwordOffset] & (uint(1) << lightDwordBitOffset))
        {
            ++numLightsInTile;
        }
    }
    
    float3 srcColor = inputTex[DTid.xy].rgb;
    float3 debugColor;
    if (numLightsInTile == 0)
    {
        debugColor = srcColor;
    }
    else if (numLightsInTile <= 3)
    {
        debugColor = ReallyLowDensityTileColor;
    }
    else if (numLightsInTile <= 10)
    {
        debugColor = LowDensityTileColor;
    }
    else if (numLightsInTile <= 17)
    {
        debugColor = MidDensityTileColor;
    }
    else if (numLightsInTile <= 32)
    {
        debugColor = HighDensityTileColor;
    }
    else
    {
        debugColor = ReallyHighDensityTileColor;
    }
    
    float3 dstColor = srcColor * debugColor;
    outputTex[DTid.xy] = float4(dstColor, 1.f);
}