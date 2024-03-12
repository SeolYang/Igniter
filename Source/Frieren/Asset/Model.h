#pragma once
#include <Asset/Common.h>
#include <Core/Handle.h>

namespace fe
{
	class GpuBuffer;
	class GpuView;

	/* Static Mesh */
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
		bool bGenerateUvCoors = false;
		bool bFlipUvs = false;
		bool bFlipWindingOrder = false; /* IF bFlipWindingOrder: CCW -> CW */
		bool bGenerateBoundingBoxes = false;

	};

	json& operator<<(json& archive, const StaticMeshImportConfig& config);
	const json& operator>>(const json& archive, StaticMeshImportConfig& config);

	struct StaticMeshLoadConfig : public ResourceMetadata<1>
	{
		friend json& operator<<(json& archive, const StaticMeshLoadConfig& config);
		friend const json& operator>>(const json& archive, StaticMeshLoadConfig& config);

	public:
		virtual json& Serialize(json& archive) const override;
		virtual const json& Deserialize(const json& archive) override;

	public:
		size_t NumVertices{ 0 };
		size_t NumIndices{ 0 };

	};

	json& operator<<(json& archive, const StaticMeshLoadConfig& config);
	const json& operator>>(const json& archive, StaticMeshLoadConfig& config);

	struct StaticMesh
	{
	public:
		StaticMeshLoadConfig LoadConfig;
		Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> VertexBufferInstance;
		Handle<GpuView, GpuViewManager*> VertexBufferSRV; 
		Handle<GpuBuffer, DeferredDestroyer<GpuBuffer>> IndexBufferInstance;
		Handle<GpuView, GpuViewManager*> IndexBufferView;
	};

	/* Skeletal Mesh */
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

	/* Importer & Loader */
	class ModelImporter
	{
	public:
		/* #wip_todo Impl Import as Static Meshes */
		std::vector<xg::Guid> ImportAsStatic(const String resPathStr, std::optional<StaticMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
		/* #todo Impl import as Skeletal Meshes */
		std::vector<xg::Guid> ImportAsSkeletal(const String resPathStr, std::optional<SkeletalMeshImportConfig> config = std::nullopt, const bool bIsPersistent = false);
	};

	class StaticMeshLoader
	{
	public:
		std::optional<StaticMesh> Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
	};
} // namespace fe