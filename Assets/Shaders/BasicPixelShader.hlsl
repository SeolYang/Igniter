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
    float4 aPos : SV_POSITION;
    float2 aUv : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[perDrawData.PerFrameDataCbv];
	StructuredBuffer<Material> materialStorage = ResourceDescriptorHeap[perFrameData.MaterialStorageSrv];
	Material materialData = materialStorage[perDrawData.MaterialIdx];

    Texture2D    texture      = ResourceDescriptorHeap[materialData.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[materialData.DiffuseTexSampler]; // test code material에 sampler도 넣어야함

	return float4(pow(texture.Sample(samplerState, input.aUv).rgb, 1.f/2.2f), 1.f);
}
