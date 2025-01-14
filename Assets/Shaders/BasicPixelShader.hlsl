struct PerFrameData
{
	float4x4 View;
	float4x4 ViewProj;
    
	float4 CamPosInvAspectRatio;
	float4 ViewFrustumParams;

	uint StaticMeshVertexStorageSrv;
	uint VertexIndexStorageSrv;

	uint TransformStorageSrv;
	uint MaterialStorageSrv;
	uint MeshStorageSrv;

	uint InstancingDataStorageSrv;
	uint InstancingDataStorageUav;

	uint TransformIdxStorageSrv;
	uint TransformIdxStorageUav;

	uint RenderableStorageSrv;

	uint RenderableIndicesBufferSrv;
	uint NumMaxRenderables;

	uint PerFrameDataCbv;

	uint3 Padding;
};

struct PerDrawData
{
    uint PerFrameDataCbv;
    uint InstancingId;
    uint MaterialIdx;
    uint VertexOffset;
    uint VertexIdxStorageOffset;
    uint TransformIdxStorageOffset;
};

struct MaterialData
{
    uint DiffuseTexSrv;
    uint DiffuseTexSampler;
};

struct InstancingData
{
    uint MaterialDataIdx;
    uint MeshDataIdx;
    uint InstanceId;
    uint TransformOffset;
    uint MaxNumInstances;
    uint NumInstances;
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
    StructuredBuffer<MaterialData> materialStorage = ResourceDescriptorHeap[perFrameData.MaterialStorageSrv];
    MaterialData materialData = materialStorage[perDrawData.MaterialIdx];

    Texture2D    texture      = ResourceDescriptorHeap[materialData.DiffuseTexSrv];
    SamplerState samplerState = SamplerDescriptorHeap[materialData.DiffuseTexSampler]; // test code material에 sampler도 넣어야함

	return float4(pow(texture.Sample(samplerState, input.aUv).rgb, 1.f/2.2f), 1.f);
}
