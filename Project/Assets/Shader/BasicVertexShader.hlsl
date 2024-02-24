struct PositionBuffer
{
	float3 aPosition;
};

struct BasicRenderResources
{
	uint positionBufferIdx;
	uint vertexBufferIdx;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

float4 main( uint vertexID : SV_VertexID ) : SV_POSITION
{
	ConstantBuffer<PositionBuffer> posBuffer = ResourceDescriptorHeap[renderResource.positionBufferIdx];
	StructuredBuffer<PositionBuffer> vertexBuffer = ResourceDescriptorHeap[renderResource.vertexBufferIdx];
	return float4((vertexBuffer[vertexID].aPosition + posBuffer.aPosition), 1.f);
}