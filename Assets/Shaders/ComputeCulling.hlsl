struct ComputeCullingConstants
{
	uint PerFrameDataCbv;
};

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

float ExtractMaxAbsScale(float4x4 toWorld)
{
	float xScale = length(float3(toWorld._m00, toWorld._m01, toWorld._m02));
	float yScale = length(float3(toWorld._m10, toWorld._m11, toWorld._m12));
	float zScale = length(float3(toWorld._m20, toWorld._m21, toWorld._m22));
	
	return max(max(xScale, yScale), zScale);
}

bool IsVisible(float invAspectRatio, float4 viewFrusumParams, float3 bsCentroid, float bsRadius, float4x4 toWorld, float4x4 toView)
{
	// View 행렬은 카메라가 정의하는 3개의 unit basis vector에 의해 정의되기 때문에
	// Scale에 영향을 끼치지 않는다
	float bsViewRadius = bsRadius * ExtractMaxAbsScale(toWorld);
	float4 bsWorldCentroid = mul(float4(bsCentroid, 1.f), toWorld);
	float4 bsViewCentroid = mul(bsWorldCentroid, toView);
	
	float leftPlaneSignedDist =
		(invAspectRatio * viewFrusumParams.x * bsViewCentroid.x) + (viewFrusumParams.y * bsViewCentroid.z);
	float rightPlaneSignedDist =
		(-invAspectRatio * viewFrusumParams.x * bsViewCentroid.x) + (viewFrusumParams.y * bsViewCentroid.z);
	float bottomPlaneSignedDist =
		(viewFrusumParams.x * bsViewCentroid.y) + (viewFrusumParams.y * bsViewCentroid.z);
	float topPlaneSignedDist =
		(-viewFrusumParams.x * bsViewCentroid.y) + (viewFrusumParams.y * bsViewCentroid.z);
	
	float nearPlaneSignedDist = bsViewCentroid.z - viewFrusumParams.z;
	float farPlaneSignedDist = -bsViewCentroid.z + viewFrusumParams.w;
	
	return nearPlaneSignedDist > -bsViewRadius &&
		farPlaneSignedDist > -bsViewRadius &&
		leftPlaneSignedDist > -bsViewRadius &&
		rightPlaneSignedDist > -bsViewRadius &&
		bottomPlaneSignedDist > -bsViewRadius &&
		topPlaneSignedDist > -bsViewRadius;
}

struct MeshData
{
	uint VertexOffset;
	uint NumVertices;
	uint IndexOffset;
	uint NumIndices;

	float3 BsCentroid;
	float BsRadius;
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

struct RenderableData
{
	uint Type;
	uint DataIdx;
	uint TransformIdx;
};

struct TransformData
{
	float4 Cols[3];
};

#define RENDERABLE_TYPE_STATIC_MESH 0

ConstantBuffer<ComputeCullingConstants> gComputeCullingConstantsBuffer : register(b0);

[numthreads(16, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gComputeCullingConstantsBuffer.PerFrameDataCbv];

	if (DTid.x >= perFrameData.NumMaxRenderables)
	{
		return;
	}

	StructuredBuffer<uint> renderableIndices = ResourceDescriptorHeap[perFrameData.RenderableIndicesBufferSrv];
	uint renderableIdx = renderableIndices[DTid.x];
	StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
	RenderableData renderableData = renderableStorage[renderableIdx];
    
    // 무조건 통과한다고 일단 가정.
	if (renderableData.Type == RENDERABLE_TYPE_STATIC_MESH)
	{
		RWStructuredBuffer<InstancingData> instancingDataStorage =
            ResourceDescriptorHeap[perFrameData.InstancingDataStorageUav];
		StructuredBuffer<MeshData> meshDataStorage =
            ResourceDescriptorHeap[perFrameData.MeshStorageSrv];
		StructuredBuffer<TransformData> transformStorage =
			ResourceDescriptorHeap[perFrameData.TransformStorageSrv];

		TransformData transformData = transformStorage[renderableData.TransformIdx];
		float4x4 toWorld = transpose(float4x4(transformData.Cols[0], transformData.Cols[1], transformData.Cols[2], float4(0.f, 0.f, 0.f, 1.f)));
		InstancingData instancingData = instancingDataStorage.Load(renderableData.DataIdx);
		MeshData mesh = meshDataStorage[instancingData.MeshDataIdx];
		if (perFrameData.Padding.x == 0 || IsVisible(perFrameData.CamPosInvAspectRatio.w, perFrameData.ViewFrustumParams, mesh.BsCentroid, mesh.BsRadius, toWorld, perFrameData.View))
		{
			RWStructuredBuffer<uint> transformIdxStorage = ResourceDescriptorHeap[perFrameData.TransformIdxStorageUav];
			uint transformIdx = 0;
			uint originTransformIdx = 0;
			InterlockedAdd(instancingDataStorage[renderableData.DataIdx].NumInstances, 1, transformIdx);
			InterlockedExchange(transformIdxStorage[instancingData.TransformOffset + transformIdx], renderableData.TransformIdx, originTransformIdx);
		}
	}
}
