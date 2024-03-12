#pragma once
#include <Asset/Common.h>
#include <Core/Handle.h>

namespace fe
{
	class GpuBuffer;
	class GpuView;

	struct StaticMeshImportConfig : public ResourceMetadata<1>
	{
		friend json& operator<<(json& archive, const StaticMeshImportConfig& config);
		friend const json& operator>>(const json& archive, StaticMeshImportConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
		bool bMakeLeftHanded = true;
		bool bGenerateNormals = false;
		bool bSplitLargeMeshes = false;
		bool bPreTransformVertices = false;
		bool bImproveCacheLocality = false;
		bool bGenerateUVCoords = false;
		bool bFlipUVs = false;
		bool bFlipWindingOrder = false; /* If bFlipWindingOrder => CCW -> CW */
		bool bGenerateBoundingBoxes = false;
		bool bImportTextures = false;  /* Only if textures does not imported before. */
		bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */
		/* 
		* If enabled, Ignore 'bSplitLargeMeshes' and force enable 'PreTransformVertices'.
		* It could be cause of cache inefficiency and loss of material details.
		*/
		bool bMergeMeshes = false;	   
	};

	json& operator<<(json& archive, const StaticMeshImportConfig& config);
	const json& operator>>(const json& archive, StaticMeshImportConfig& config);

	struct StaticMeshLoadConfig : public AssetMetadata<2>
	{
		friend json& operator<<(json& archive, const StaticMeshLoadConfig& config);
		friend const json& operator>>(const json& archive, StaticMeshLoadConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
		std::string Name{};
		size_t NumVertices{ 0 };
		size_t NumIndices{ 0 };
		size_t CompressedVerticesSizeInBytes{ 0 };
		size_t CompressedIndicesSizeInBytes{ 0 };
		/* #sy_todo Add AABB Info */
		// std::vector<xg::Guid> ... or std::vector<std::string> materials; Material?
	};

	json& operator<<(json& archive, const StaticMeshLoadConfig& config);
	const json& operator>>(const json& archive, StaticMeshLoadConfig& config);

	struct SkeletalMeshImportConfig : public ResourceMetadata<1>
	{
		friend json& operator<<(json& archive, const SkeletalMeshImportConfig& config);
		friend const json& operator>>(const json& archive, SkeletalMeshImportConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
	};

	json& operator<<(json& archive, const SkeletalMeshImportConfig& config);
	const json& operator>>(const json& archive, SkeletalMeshImportConfig& config);

	struct SkeletalMeshLoadConfig : public ResourceMetadata<1>
	{
		friend json& operator<<(json& archive, const SkeletalMeshLoadConfig& config);
		friend const json& operator>>(const json& archive, SkeletalMeshLoadConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
	};

	json& operator<<(json& archive, const SkeletalMeshLoadConfig& config);
	const json& operator>>(const json& archive, SkeletalMeshLoadConfig& config);

	class TextureImporter;
	class ModelImporter
	{
	public:
		/* #sy_wip_todo Impl Import as Static Meshes */
		static std::vector<xg::Guid> ImportAsStatic(TextureImporter& textureImporter, const String resPathStr, std::optional<StaticMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
		/* #sy_todo Impl import as Skeletal Meshes */
		static std::vector<xg::Guid> ImportAsSkeletal(const String resPathStr, std::optional<SkeletalMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
	};

	/* Static Mesh */
	struct StaticMesh
	{
	public:
		StaticMeshLoadConfig LoadConfig;
		Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> VertexBufferInstance;
		Handle<GpuView, GpuViewManager*> VertexBufferSRV;
		Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> IndexBufferInstance;
		Handle<GpuView, GpuViewManager*> IndexBufferView;
		// RefHandle<Material>
	};

	class StaticMeshLoader
	{
	public:
		std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
	};
} // namespace fe