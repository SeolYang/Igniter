struct PositionBuffer
{
	float3 aPos;
};

struct Vertex
{
	float3 aPos;
	float2 aUv;
};

struct BasicRenderResources
{
	uint positionBufferIdx;
	uint vertexBufferIdx;
	uint diffuseTexIdx;
	uint samplerIdx;
};

struct VertexShaderOutput
{
	float4 aPos : SV_POSITION;
	float2 aUv : TEXCOORD;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

VertexShaderOutput main( uint vertexID : SV_VertexID )
{
	ConstantBuffer<PositionBuffer> posBuffer = ResourceDescriptorHeap[renderResource.positionBufferIdx];
	StructuredBuffer<Vertex> vertexBuffer = ResourceDescriptorHeap[renderResource.vertexBufferIdx];

	VertexShaderOutput output;
	output.aPos = float4((vertexBuffer[vertexID].aPos + posBuffer.aPos), 1.f);
	output.aUv = vertexBuffer[vertexID].aUv;

	return output;
}