struct PerFrameBuffer
{
    float4x4 ViewProj;
};

struct PerObjectBuffer
{
    float4x4 LocalToWorld;
};

struct Vertex
{
    float3 aPos;
    float3 aNormal;
    float2 aUv;
};

struct BasicRenderResources
{
    uint VertexBufferIdx;
    uint PerFrameBufferIdx;
    uint PerObjectBufferIdx;
    uint DiffuseTexIdx;
    uint SamplerIdx;
};

struct VertexShaderOutput
{
    float4 aPos : SV_POSITION;
    float2 aUv : TEXCOORD;
};

ConstantBuffer<BasicRenderResources> renderResource : register(b0);

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    ConstantBuffer<PerFrameBuffer>  perFrameBuffer  = ResourceDescriptorHeap[renderResource.PerFrameBufferIdx];
    ConstantBuffer<PerObjectBuffer> perObjectBuffer = ResourceDescriptorHeap[renderResource.PerObjectBufferIdx];
    StructuredBuffer<Vertex>        vertexBuffer    = ResourceDescriptorHeap[renderResource.VertexBufferIdx];

    float4x4 worldViewProj = mul(perObjectBuffer.LocalToWorld, perFrameBuffer.ViewProj);

    VertexShaderOutput output;
    output.aPos = mul(float4(vertexBuffer[vertexID].aPos.xyz, 1.f), worldViewProj);
    output.aUv  = vertexBuffer[vertexID].aUv;
    return output;
}
