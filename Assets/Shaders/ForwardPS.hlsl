#include "MeshInstancePass.hlsli"

ConstantBuffer<DispatchMeshInstanceParams> gParams : register(b0);

[earlydepthstencil]
float4 main(VertexOutput input) : SV_Target
{
    ConstantBuffer<PerFrameParams> perFrameParams = ResourceDescriptorHeap[gParams.PerFrameParamsCbv];
    ConstantBuffer<SceneProxyConstants> sceneProxyConstants = ResourceDescriptorHeap[gParams.SceneProxyConstantsCbv];
    ConstantBuffer<LightClusterParams> lightClusterParams = ResourceDescriptorHeap[perFrameParams.LightClusterParamsCbv];
    StructuredBuffer<uint> lightIdxList = ResourceDescriptorHeap[lightClusterParams.LightIdxListSrv];
    StructuredBuffer<uint> tileBitfields = ResourceDescriptorHeap[lightClusterParams.TileBitfieldsSrv];
    StructuredBuffer<uint2> depthBins = ResourceDescriptorHeap[lightClusterParams.DepthBinsSrv];
    StructuredBuffer<Light> lightStorage = ResourceDescriptorHeap[sceneProxyConstants.LightStorageSrv];
    StructuredBuffer<MeshInstance> meshInstanceStorage = ResourceDescriptorHeap[sceneProxyConstants.MeshInstanceStorageSrv];
    StructuredBuffer<Material> materialStorage = ResourceDescriptorHeap[sceneProxyConstants.MaterialStorageSrv];
    
    MeshInstance meshInstance = meshInstanceStorage[gParams.MeshInstanceIdx];
    Material material = materialStorage[meshInstance.MaterialProxyIdx];

    const float3 normal = normalize(input.Normal);
    Texture2D texture = ResourceDescriptorHeap[material.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[material.DiffuseTexSampler]; // test code material에 sampler도 넣어야함
    const float3 diffuse = texture.Sample(samplerState, input.TexCoord0).rgb;

    float ld = LinearizeDepthReverseZ(input.Position.z, perFrameParams.ViewFrustumParams.z, perFrameParams.ViewFrustumParams.w) / (perFrameParams.ViewFrustumParams.w - perFrameParams.ViewFrustumParams.z);
    uint depthBinIdx = uint(MAX_DEPTH_BIN_IDX_F32 * ld);
    int tileX = int(input.Position.x * INV_TILE_SIZE);
    int tileY = int(input.Position.y * INV_TILE_SIZE);

    uint2 depthBin = depthBins[depthBinIdx];
    const uint numTileX = uint(perFrameParams.ViewportWidth * INV_TILE_SIZE);
    const uint tileDwordOffset = ((numTileX * tileY) + tileX) * NUM_U32_PER_TILE;
    float3 illuminance = float3(0.f, 0.f, 0.f);
 
    uint mergedMinIdx = WaveActiveMin(depthBin.x);
    uint mergedMaxIdx = WaveActiveMax(depthBin.y);
    uint wordMin = max(mergedMinIdx / 32, 0);
    uint wordMax = min(mergedMaxIdx / 32, NUM_U32_PER_TILE - 1);
    for (uint wordIdx = wordMin; wordIdx <= wordMax; ++wordIdx)
    {
        uint mask = tileBitfields[tileDwordOffset + wordIdx];
        // wordIdx * 32 = 결국 현재 시점에서의 lightidxlistidx
        uint localMin = clamp((int)depthBin.x - (int)(wordIdx * 32), 0, 31);
        uint maskWidth = clamp((int)depthBin.y - (int)(wordIdx * 32) + 1, 0, 32);
        // depthbin의 범위에 따라, 현재 word(또는 window)에서의 어떤 영역을 선택 할 것인지 알아내는 것이다
        uint zBinMask = BitfieldMask(maskWidth, localMin);
        // 아래가 기존 코드
        //uint maskWidth = clamp((int)depthBin.y - (int)depthBin.x + 1, 0, 32);
        //uint zBinMask = maskWidth == 32 ? 0xFFFFFFFF : BitfieldMask(maskWidth, localMin);
        mask &= zBinMask;
        // 이 시점에서 merged Mask는 Wave가 word 내에서 처리해야할 전체 광원에 대한 비트를 가지고 잇음
        uint mergedMask = WaveActiveBitOr(mask);
        while (mergedMask != 0)
        {
            uint bitIdx = firstbitlow(mergedMask);
            mergedMask ^= (uint(1) << bitIdx);
            uint lightIdxListIdx = 32 * wordIdx + bitIdx;

            uint lightIdx = lightIdxList[lightIdxListIdx];
            Light light = lightStorage[lightIdx];
            const float3 toLight = light.WorldPos - input.WorldPosition;
            const float distSquared = dot(toLight, toLight);
            const float lightinvRadius = 1.f / (light.FalloffRadius * light.FalloffRadius);
            const float factor = distSquared * lightinvRadius;
            const float smoothFactor = max(1.0 - factor * factor, 0.0);
            const float att = (smoothFactor * smoothFactor) / max(distSquared, 1e-4);
            illuminance += light.Color * light.Intensity * att * saturate(dot(normalize(toLight), normal));
        }
    }

    return float4(pow(illuminance * diffuse, 1.f / 2.2f), 1.f);
}
