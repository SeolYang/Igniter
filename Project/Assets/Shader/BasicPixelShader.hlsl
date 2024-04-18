struct BasicRenderResources
{
    uint VertexBufferIdx;
    uint PerFrameBufferIdx;
    uint PerObjectBufferIdx;
    uint DiffuseTexIdx;
    uint SamplerIdx;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

struct PixelShaderInput
{
    float4 aPos : SV_POSITION;
    float2 aUv : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    Texture2D    texture      = ResourceDescriptorHeap[renderResource.DiffuseTexIdx];
    SamplerState samplerState = SamplerDescriptorHeap[renderResource.SamplerIdx];

	return float4(pow(texture.Sample(samplerState, input.aUv).rgb, 1.f/2.2f), 1.f);
}
