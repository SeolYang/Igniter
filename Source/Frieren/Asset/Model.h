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
		bool bFlipUVs = true;
		bool bFlipWindingOrder = true;
		bool bGenerateNormals = false;
		bool bSplitLargeMeshes = false;
		bool bPreTransformVertices = false;
		bool bImproveCacheLocality = false;
		bool bGenerateUVCoords = false;
		bool bGenerateBoundingBoxes = false;

		/*
		 * If enabled, Ignore 'bSplitLargeMeshes' and force enable 'PreTransformVertices'.
		 * It cause of loss material details.
		 */
		bool bMergeMeshes = false;

		bool bImportTextures = false;  /* Only if textures does not imported before. */
		bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */
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
		uint32_t NumVertices{ 0 };
		uint32_t NumIndices{ 0 };
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
		Handle<GpuView, GpuViewManager*> VertexBufferSrv;
		Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> IndexBufferInstance;
		// RefHandle<Material>
	};

	class StaticMeshLoader
	{
	public:
		static std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
	};
} // namespace fe