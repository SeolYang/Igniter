#include "Common.hlsl"

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint MaterialIdx;
    uint VertexOffset;
    uint VertexIdxOffset;
    uint IndirectTransformOffset;
};
ConstantBuffer<PerDrawData> perDrawData : register(b0);

struct PixelShaderInput
{
    float4 aPos : SV_POSITION; // SCREEN SPACE
    float2 aUv : TEXCOORD;
    float3 aNormal : NORMAL;
    float3 aWorldPos : WORLD_POS;
};

[earlydepthstencil]
float4 main(PixelShaderInput input) : SV_TARGET
{
    const float3 normal = normalize(input.aNormal);
    
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[perDrawData.PerFrameDataCbv];
	
    StructuredBuffer<uint> lightIdxList = ResourceDescriptorHeap[perFrameData.LightIdxListSrv];
    StructuredBuffer<uint> tileBitfields = ResourceDescriptorHeap[perFrameData.LightTileBitfieldBufferSrv];
    StructuredBuffer<uint2> depthBins = ResourceDescriptorHeap[perFrameData.LightDepthBinBufferSrv];
    StructuredBuffer<Light> lightStorage = ResourceDescriptorHeap[perFrameData.LightStorageSrv];
    StructuredBuffer<Material> materialStorage = ResourceDescriptorHeap[perFrameData.MaterialStorageSrv];
    Material materialData = materialStorage[perDrawData.MaterialIdx];

    Texture2D texture = ResourceDescriptorHeap[materialData.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[materialData.DiffuseTexSampler]; // test code material에 sampler도 넣어야함
    
    float ld = LinearizeDepthReverseZ(input.aPos.z, perFrameData.ViewFrustumParams.z, perFrameData.ViewFrustumParams.w) / (perFrameData.ViewFrustumParams.w - perFrameData.ViewFrustumParams.z);
    //return float4(ld, ld, ld, 1.f);
    int depthBinIdx = int(MAX_DEPTH_BIN_IDX_F32 * ld);
    
    int tileX = int(input.aPos.x * INV_TILE_SIZE);
    int tileY = int(input.aPos.y * INV_TILE_SIZE);
    uint2 depthBin = depthBins[depthBinIdx];
    const uint numTileX = uint(2560.f * INV_TILE_SIZE);
    const uint tileDwordOffset = ((numTileX * tileY) + tileX) * NUM_U32_PER_TILE;
    
    float3 illuminance = float3(0.f, 0.f, 0.f);
    const uint minLightIdxListIdx = WaveActiveMin(depthBin.x);
    const uint maxLightIdxListIdx = WaveActiveMax(depthBin.y);
    const uint currentLaneMask = uint(1) << WaveGetLaneIndex();
    for (uint lightIdxListIdx = minLightIdxListIdx; lightIdxListIdx <= maxLightIdxListIdx; ++lightIdxListIdx)
    {
        const uint lightIdxListIdxMask = WaveActiveBallot(lightIdxListIdx >= depthBin.x && lightIdxListIdx <= depthBin.y).x;
        if ((currentLaneMask & lightIdxListIdxMask) == 0)
        {
            continue;
        }
        
        const uint s_lightDword = (uint(1) << (lightIdxListIdx % 32));
        const uint lightDwordOffset = tileDwordOffset + (lightIdxListIdx / 32);
        if ((tileBitfields[lightDwordOffset] & s_lightDword) == 0)
        {
            continue;
        }
       
        const uint s_lightIdx = lightIdxList[lightIdxListIdx];
        const Light s_light = lightStorage[s_lightIdx];
        const float3 toLight = s_light.WorldPos - input.aWorldPos;
        const float distSquared = dot(toLight, toLight);
        //const float att = 1.f / (4.f * PI * distSquared);
        const float lightinvRadius = 1.f / s_light.Radius;
        float factor = distSquared * lightinvRadius * lightinvRadius;
        float smoothFactor = max(1.0 - factor * factor, 0.0);
        const float att = (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
        illuminance += s_light.Color * s_light.Intensity * att * dot(normalize(toLight), normal);
    }
    
    //for (uint lightIdxListIdx = depthBin.x; lightIdxListIdx <= depthBin.y; ++lightIdxListIdx)
    //{
    //    const uint s_lightDword = (uint(1) << (lightIdxListIdx % 32));
    //    const uint lightDwordOffset = tileDwordOffset + (lightIdxListIdx / 32);
    //    if ((tileBitfields[lightDwordOffset] & s_lightDword) == 0)
    //    {
    //        continue;
    //    }
       
    //    const uint s_lightIdx = lightIdxList[lightIdxListIdx];
    //    const Light s_light = lightStorage[s_lightIdx];
    //    const float3 toLight = s_light.WorldPos - input.aWorldPos;
    //    const float distSquared = dot(toLight, toLight);
    //    const float att = 1.f / (4.f * PI * distSquared);
    //            //const float lightinvRadius = 1.f / s_light.Radius;
    //            //float factor = distSquared * lightinvRadius * lightinvRadius;
    //            //float smoothFactor = max(1.0 - factor * factor, 0.0);
    //            //const float att = (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
    //    illuminance += s_light.Color * s_light.Intensity * att * dot(normalize(toLight), normal);
    //}
    
    return float4(pow(illuminance * texture.Sample(samplerState, input.aUv).rgb, 1.f / 2.2f), 1.f);
}
