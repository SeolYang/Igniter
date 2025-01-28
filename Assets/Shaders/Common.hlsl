#define MAX_MESH_LOD 8
#define RENDERABLE_TYPE_STATIC_MESH 0
#define FLT_MAX 3.402823466e+38F

struct PerFrameData
{
	float4x4 ToView;
	float4x4 ToViewProj;
    
	/* (x,y,z): cam pos, w: inv aspect ratio */
	float4 CamPosInvAspectRatio;
	/* x: cos(fovy/2), y: sin(fovy/2), z: near, w: far */
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

struct Light
{
    uint Type;
    float Radius;
    float3 WorldPos;
    float3 Forward;
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
