struct PerFrameData
{
    float4x4 ViewProj;
    float4 CameraPos;
    float4 CameraForward;

    uint StaticMeshVertexStorageSrv;
    uint VertexIndexStorageSrv;

    uint TransformStorageSrv;
    uint MaterialStorageSrv;
    uint MeshStorageSrv;

    uint StaticMeshStorageSrv;

    uint RenderableStorageSrv;

    uint RenderableIndicesBufferSrv;
    uint NumMaxRenderables;

    uint PerFrameDataCbv;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint RenderableDataIdx;
};

struct MaterialData
{
    uint DiffuseTexSrv;
    uint DiffuseTexSampler;
};

struct RenderableData
{
    uint Type;
    uint DataIdx;
    uint TransformIdx;
};

struct StaticMeshData
{
    uint MaterialIdx;
    uint MeshIdx;
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
    StructuredBuffer<RenderableData> renderableDataStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
    StructuredBuffer<StaticMeshData> staticMeshStorage = ResourceDescriptorHeap[perFrameData.StaticMeshStorageSrv];
    RenderableData renderableData = renderableDataStorage[perDrawData.RenderableDataIdx];
    StaticMeshData staticMeshData = staticMeshStorage[renderableData.DataIdx];

    StructuredBuffer<MaterialData> materialStorage = ResourceDescriptorHeap[perFrameData.MaterialStorageSrv];
    MaterialData materialData = materialStorage[staticMeshData.MaterialIdx];

    Texture2D    texture      = ResourceDescriptorHeap[materialData.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[materialData.DiffuseTexSampler]; // test code material에 sampler도 넣어야함

	return float4(pow(texture.Sample(samplerState, input.aUv).rgb, 1.f/2.2f), 1.f);
}
