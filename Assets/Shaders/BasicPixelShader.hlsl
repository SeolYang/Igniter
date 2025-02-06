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

uint BitfieldMask(uint width, uint min)
{
    return (uint(0xFFFFFFFF) >> width) << min;
}

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
    const float3 diffuse = texture.Sample(samplerState, input.aUv).rgb;
    
    float ld = LinearizeDepthReverseZ(input.aPos.z, perFrameData.ViewFrustumParams.z, perFrameData.ViewFrustumParams.w) / (perFrameData.ViewFrustumParams.w - perFrameData.ViewFrustumParams.z);
    uint depthBinIdx = uint(MAX_DEPTH_BIN_IDX_F32 * ld);
    int tileX = int(input.aPos.x * INV_TILE_SIZE);
    int tileY = int(input.aPos.y * INV_TILE_SIZE);
    
    uint2 depthBin = depthBins[depthBinIdx];
    const uint numTileX = uint(2560.f * INV_TILE_SIZE);
    const uint tileDwordOffset = ((numTileX * tileY) + tileX) * NUM_U32_PER_TILE;
    float3 illuminance = float3(0.f, 0.f, 0.f);
    //uint lightIdxListIdx = WaveActiveMin(depthBin.x);
    //const uint lastLightIdxListIdx = WaveActiveMax(depthBin.y);
    //while (lightIdxListIdx <= lastLightIdxListIdx)
    //{
    //    ++lightIdxListIdx;
        
    //    if (lightIdxListIdx >= depthBin.x && lightIdxListIdx <= depthBin.y)
    //    {
    //        const uint s_lightDword = (uint(1) << (lightIdxListIdx % 32));
    //        const uint s_lightDwordOffset = lightIdxListIdx / 32;
            
    //        const uint v_tileBitfield = tileBitfields.Load(tileDwordOffset + s_lightDwordOffset);
    //        bool bIsVisibleLight = (s_lightDword & v_tileBitfield) != 0;
    //        if (!bIsVisibleLight)
    //        {
    //            continue;
    //        }
            
    //        uint s_lightIdx;
    //        Light s_light;
    //        if (WaveIsFirstLane())
    //        {
    //            s_lightIdx = lightIdxList[lightIdxListIdx];
    //            s_light = lightStorage[s_lightIdx];
    //        }
    //        s_light.WorldPos = WaveReadLaneFirst(s_light.WorldPos);
    //        s_light.Radius = WaveReadLaneFirst(s_light.Radius);
    //        s_light.Color = WaveReadLaneFirst(s_light.Color);
    //        s_light.Intensity = WaveReadLaneFirst(s_light.Intensity);
        
    //        const float3 toLight = s_light.WorldPos - input.aWorldPos;
    //        const float distSquared = dot(toLight, toLight);
    //        const float lightinvRadius = 1.f / (s_light.Radius * s_light.Radius);
    //        const float factor = distSquared * lightinvRadius;
    //        const float smoothFactor = max(1.0 - factor * factor, 0.0);
    //        const float att = (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
    //        illuminance += s_light.Color * s_light.Intensity * att * saturate(dot(normalize(toLight), normal));
    //    }
    //}
    
    uint mergedMinIdx = WaveActiveMin(depthBin.x);
    uint mergedMaxIdx = WaveActiveMax(depthBin.y);
    uint wordMin = max(mergedMinIdx / 32, 0);
    uint wordMax = min(mergedMaxIdx / 32, NUM_U32_PER_TILE - 1);
    uint maskWidth = clamp((int) depthBin.y - (int) depthBin.x + 1, 0, 32);
    for (uint wordIdx = wordMin; wordIdx <= wordMax; ++wordIdx)
    {
        uint mask = tileBitfields[tileDwordOffset + wordIdx];
        uint localMin = clamp((int) depthBin.x - (int) (wordIdx * 32), 0, 31);
        uint zBinMask = maskWidth == 32 ? 0xFFFFFFFF : BitfieldMask(maskWidth, localMin);
        mask &= zBinMask;
        uint mergedMask = WaveActiveBitOr(mask);
        while (mergedMask != 0)
        {
            uint bitIdx = firstbitlow(mergedMask);
            mergedMask ^= (1 << bitIdx);
            uint lightIdxListIdx = 32 * wordIdx + bitIdx;
            
            uint lightIdx = lightIdxList[lightIdxListIdx];
            Light light = lightStorage[lightIdx];
            const float3 toLight = light.WorldPos - input.aWorldPos;
            const float distSquared = dot(toLight, toLight);
            const float lightinvRadius = 1.f / (light.Radius * light.Radius);
            const float factor = distSquared * lightinvRadius;
            const float smoothFactor = max(1.0 - factor * factor, 0.0);
            const float att = (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
            illuminance += light.Color * light.Intensity * att * saturate(dot(normalize(toLight), normal));
        }
    }
    
    return float4(pow(illuminance * diffuse, 1.f / 2.2f), 1.f);
}
