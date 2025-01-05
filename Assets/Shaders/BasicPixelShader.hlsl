struct PerFrameData
{
    float4x4 ViewProj;
    float4 CameraPos;
    float4 CameraForward;

    uint StaticMeshVertexStorageSrv;
    uint VertexIndexStorageSrv;

    uint TransformStorageSrv;
    uint MaterialStorageSrv;
    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint RenderableIdx;
};

struct MaterialData
{
    uint DiffuseTexSrv;
    uint DiffuseTexSampler;
};

struct RenderableData
{
    uint TransformIdx;
    uint MaterialIdx;
    uint VertexOffset; // deprecated
    uint NumVertices; // deprecated
    uint IndexOffset;
    uint NumIndices;
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
    StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
    RenderableData renderableData = renderableStorage[perDrawData.RenderableIdx];

    StructuredBuffer<MaterialData> materialStorage = ResourceDescriptorHeap[perFrameData.MaterialStorageSrv];
    MaterialData materialData = materialStorage[renderableData.MaterialIdx];

    Texture2D    texture      = ResourceDescriptorHeap[materialData.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[materialData.DiffuseTexSampler]; // test code material에 sampler도 넣어야함

	return float4(pow(texture.Sample(samplerState, input.aUv).rgb, 1.f/2.2f), 1.f);
}
