#define MAX_MESH_LOD 8
#define RENDERABLE_TYPE_STATIC_MESH 0

struct PerFrameData
{
	float4x4 ToView;
	float4x4 ToViewProj;
    
	float4 CamPosInvAspectRatio;
	float4 ViewFrustumParams;
	uint EnableFrustumCulling;
	// test purpose
	uint MinMeshLod;

	uint StaticMeshVertexStorageSrv;
	uint VertexIndexStorageSrv;

	uint TransformStorageSrv;
	uint MaterialStorageSrv;
	uint MeshStorageSrv;

	uint InstancingDataStorageSrv;
	uint InstancingDataStorageUav;
	
	uint IndirectTransformStorageSrv;
	uint IndirectTransformStorageUav;

	uint RenderableStorageSrv;

	uint RenderableIndicesBufferSrv;
	uint NumMaxRenderables;
};

struct Material
{
	uint DiffuseTexSrv;
	uint DiffuseTexSampler;
};

struct MeshLod
{
	uint IndexOffset;
	uint NumIndices;
};

struct Mesh
{
	uint VertexOffset;
	uint NumVertices;
	
	uint NumLods;
	MeshLod Lods[MAX_MESH_LOD];

	float3 BsCentroid;
	float BsRadius;
};

struct InstancingData
{
	uint MaterialIdx;
	uint MeshIdx;
	uint InstanceId;
	uint IndirectTransformOffset;
	uint MaxNumInstances;
	uint NumVisibleLodInstances[MAX_MESH_LOD];
};

struct MeshLodInstance
{
	uint RenderableIdx;
	uint Lod;
	uint LodInstanceId; // LOD Local Instance ID
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

struct CullingData
{
	uint NumVisibleLodInstances;
};

struct DrawInstance
{
	uint PerFrameDataCbv;
	uint MaterialIdx;
	uint VertexOffset;
	uint VertexIdxOffset;
	uint IndirectTransformOffset;

	uint VertexCountPerInstance;
	uint InstanceCount;
	uint StartVertexLocation;
	uint StartInstanceLocation;
};

struct VertexSM
{
	float3 Position;
	float3 Normal;
	float2 TexCoord0;
};
