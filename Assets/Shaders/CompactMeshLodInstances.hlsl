#include "Common.hlsl"

struct CompactMeshInstancesConstants
{
	uint PerFrameCbv;
	uint MeshLodInstanceStorageUav;
	uint CullingDataBufferSrv;
};
ConstantBuffer<CompactMeshInstancesConstants> gConstants : register(b0);

[numthreads(32, 1, 1)]
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
	RWStructuredBuffer<uint> indirectTransformStorage = ResourceDescriptorHeap[perFrameData.IndirectTransformStorageUav];
	
	MeshLodInstance meshLodInstance = meshLodInstances.Consume(); // VGPR
	uint v_renderableIdx = meshLodInstance.RenderableIdx;
	uint v_lod = meshLodInstance.Lod;
	uint v_lodInstanceId = meshLodInstance.LodInstanceId;
	
	uint v_laneIdx = WaveGetLaneIndex(); // VGPR
	uint s_executeMask = 0xffffffff; // SGPR
	uint v_currentLaneMask = uint(1) << v_laneIdx; // VGPR
	while ((s_executeMask & v_currentLaneMask) != 0)
	{
		uint s_renderableIdx = WaveReadLaneFirst(v_renderableIdx);
		uint s_lod = WaveReadLaneFirst(v_lod);
		uint s_lodInstanceId = WaveReadLaneFirst(v_lodInstanceId);
		uint s_laneMask = WaveActiveBallot(s_renderableIdx == v_renderableIdx).x;
		s_executeMask = s_executeMask & ~s_laneMask;
		if (v_renderableIdx == s_renderableIdx)
		{
			RenderableData s_renderableData = renderableStorage[s_renderableIdx];
			InstancingData s_instancingData = instancingDataStorage[s_renderableData.DataIdx];
			uint s_lodIndirectTransformOffset = s_instancingData.IndirectTransformOffset; // SGPR
			for (uint lod = 0; lod < s_lod; ++lod)
			{
				s_lodIndirectTransformOffset += s_instancingData.NumVisibleLodInstances[lod]; // SGPR
			}
			
			uint s_instanceIndirectTransformOffset = s_lodIndirectTransformOffset + meshLodInstance.LodInstanceId;
			uint exchangedTransformIdx; // VGPR
			InterlockedExchange(
				indirectTransformStorage[s_instanceIndirectTransformOffset],
				s_renderableData.TransformIdx,
				exchangedTransformIdx);
		}
	}
}