#include "Common.hlsl"

struct CompactMeshInstancesConstants
{
	uint PerFrameCbv;
	uint MeshLodInstanceStorageUav;
	uint CullingDataBufferSrv;
};
ConstantBuffer<CompactMeshInstancesConstants> gConstants : register(b0);

[numthreads(16, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	StructuredBuffer<CullingData> cullingDataBuffer = ResourceDescriptorHeap[gConstants.CullingDataBufferSrv];
	if (DTid.x >= cullingDataBuffer[0].NumVisibleLodInstances)
	{
		return;
	}
	
	ConstantBuffer<PerFrameData> perFrameData = ResourceDescriptorHeap[gConstants.PerFrameCbv];
	
	ConsumeStructuredBuffer<MeshLodInstance> meshLodInstances = ResourceDescriptorHeap[gConstants.MeshLodInstanceStorageUav];
	StructuredBuffer<RenderableData> renderableStorage = ResourceDescriptorHeap[perFrameData.RenderableStorageSrv];
	StructuredBuffer<InstancingData> instancingDataStorage = ResourceDescriptorHeap[perFrameData.InstancingDataStorageSrv];
	
	MeshLodInstance meshLodInstance = meshLodInstances.Consume();
	RenderableData renderableData = renderableStorage[meshLodInstance.RenderableIdx];
	InstancingData instancingData = instancingDataStorage[renderableData.DataIdx];
	
	uint lodIndirectTransformOffset = instancingData.IndirectTransformOffset;
	for (uint lod = 0; lod < meshLodInstance.Lod; ++lod)
	{
		lodIndirectTransformOffset += instancingData.NumVisibleLodInstances[lod];
	}
	
	uint instanceIndirectTransformOffset = lodIndirectTransformOffset + meshLodInstance.LodInstanceId;
	uint exchangedTransformIdx; // maybe not use
	RWStructuredBuffer<uint> indirectTransformStorage = ResourceDescriptorHeap[perFrameData.IndirectTransformStorageUav];
	InterlockedExchange(
		indirectTransformStorage[instanceIndirectTransformOffset],
		renderableData.TransformIdx,
		exchangedTransformIdx);
}