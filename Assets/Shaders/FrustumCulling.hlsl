#include "Common.hlsl"

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

struct FrustumCullingConstants
{
	uint PerFrameDataCbv;
	uint MeshLodInstanceStorageUav;
	uint CullingDataBufferUav;
};
ConstantBuffer<FrustumCullingConstants> gConstants : register(b0);

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gConstants.PerFrameDataCbv];
	if (DTid.x >= perFrameData.NumMaxRenderables)
	{
		return;
	}
	
	StructuredBuffer<uint> renderableIndices = ResourceDescriptorHeap[perFrameData.RenderableIndicesBufferSrv];
	uint renderableIdx = renderableIndices[DTid.x];
	StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
	RenderableData renderableData = renderableStorage[renderableIdx];
	if (renderableData.Type == RENDERABLE_TYPE_STATIC_MESH)
	{
		RWStructuredBuffer<InstancingData> instancingDataStorage =
            ResourceDescriptorHeap[perFrameData.InstancingDataStorageUav];
		StructuredBuffer<Mesh> meshStorage =
            ResourceDescriptorHeap[perFrameData.MeshStorageSrv];
		StructuredBuffer<TransformData> transformStorage =
			ResourceDescriptorHeap[perFrameData.TransformStorageSrv];
		
		TransformData transformData = transformStorage[renderableData.TransformIdx];
		float4x4 toWorld = transpose(float4x4(
			transformData.Cols[0],
			transformData.Cols[1],
			transformData.Cols[2],
			float4(0.f, 0.f, 0.f, 1.f)));
		
		InstancingData instancingData = instancingDataStorage.Load(renderableData.DataIdx);
		Mesh mesh = meshStorage[instancingData.MeshIdx];
		
		if (!perFrameData.EnableFrustumCulling ||
			IsVisible(
				perFrameData.CamPosInvAspectRatio.w,
				perFrameData.ViewFrustumParams,
				mesh.BsCentroid,
				mesh.BsRadius,
				toWorld, perFrameData.ToView))
		{
			float camNearToFarDist = perFrameData.ViewFrustumParams.w - perFrameData.ViewFrustumParams.z;
			float3 meshInstancePos = float3(transformData.Cols[0].w, transformData.Cols[1].w, transformData.Cols[2].w);
			float meshInstanceToCamDist = length(meshInstancePos - perFrameData.CamPosInvAspectRatio.xyz);
			
			// 현재는 거리 기반 LOD 계산 -> 추후 Screen Space 기반 화면에서 어느정도 영역을 커버하는지 측정해서, 해당 값을 기반으로 정하는
			// 알고리즘으로 개선해보기! (만약 엄청나게 큰 메쉬일 경우를 고려해서)
			// 가능하면 Static Mesh Component에 해당 파라미터 추가하기
			MeshLodInstance newVisibleLodInstance;
			newVisibleLodInstance.RenderableIdx = renderableIdx;
			newVisibleLodInstance.Lod = uint(lerp(float(min(perFrameData.MinMeshLod, mesh.NumLods-1)), float(mesh.NumLods), saturate(meshInstanceToCamDist / camNearToFarDist)));
			InterlockedAdd(
				instancingDataStorage[renderableData.DataIdx].NumVisibleLodInstances[newVisibleLodInstance.Lod],
				1,
				newVisibleLodInstance.LodInstanceId);
			
			AppendStructuredBuffer<MeshLodInstance> meshLodInstances = ResourceDescriptorHeap[gConstants.MeshLodInstanceStorageUav];
			meshLodInstances.Append(newVisibleLodInstance);
		
			RWStructuredBuffer<CullingData> cullingDataBuffer = ResourceDescriptorHeap[gConstants.CullingDataBufferUav];
			InterlockedAdd(cullingDataBuffer[0].NumVisibleLodInstances, 1);
		}
	}
}