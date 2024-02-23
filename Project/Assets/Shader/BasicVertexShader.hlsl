struct PositionBuffer
{
	float3 aPosition;
};

struct BasicRenderResources
{
	uint positionBufferIdx;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

float4 main( float3 pos : POSITION ) : SV_POSITION
{
	ConstantBuffer<PositionBuffer> posBuffer = ResourceDescriptorHeap[renderResource.positionBufferIdx];
	return float4((pos + posBuffer.aPosition), 1.f);
}