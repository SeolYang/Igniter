struct BasicRenderResources
{
	uint positionBufferIdx;
	uint vertexBufferIdx;
	uint diffuseTexIdx;
	uint samplerIdx;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

struct PixelShaderInput
{
	float4 aPos : SV_POSITION;
	float2 aUv : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	Texture2D texture = ResourceDescriptorHeap[renderResource.diffuseTexIdx];
	SamplerState samplerState = SamplerDescriptorHeap[renderResource.samplerIdx];

	return float4(texture.Sample(samplerState, input.aUv).rgb, 1.f);
}