#include <Asset/Model.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/GpuViewManager.h>
#include <Render/GpuUploader.h>
#include <Render/Vertex.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(ModelImporterInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(ModelImporterWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(ModelImporterErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(ModelImporterFatal, ELogVerbosity::Fatal)
	FE_DEFINE_LOG_CATEGORY(StaticMeshLoaderInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(StaticMeshLoaderWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(StaticMeshLoaderErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(StaticMeshLoaderFatal, ELogVerbosity::Fatal)

	json& StaticMeshImportConfig::Serialize(json& archive) const
	{
		CommonMetadata::Serialize(archive);

		const auto& config = *this;
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMakeLeftHanded);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateNormals);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bSplitLargeMeshes);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bPreTransformVertices);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImproveCacheLocality);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUvCoors);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUvs);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipWindingOrder);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateBoundingBoxes);

		return archive;
	}

	const json& StaticMeshImportConfig::Deserialize(const json& archive)
	{
		auto& config = *this;
		config = {};

		CommonMetadata::Deserialize(archive);

		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMakeLeftHanded);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateNormals);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bSplitLargeMeshes);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bPreTransformVertices);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImproveCacheLocality);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUvCoors);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUvs);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipWindingOrder);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateBoundingBoxes);

		return archive;
	}

	json& StaticMeshLoadConfig::Serialize(json& archive) const
	{
		check(NumVertices > 0);
		check(NumIndices > 0);

		CommonMetadata::Serialize(archive);

		const auto& config = *this;
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices);
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices);

		return archive;
	}

	const json& StaticMeshLoadConfig::Deserialize(const json& archive)
	{
		auto& config = *this;
		config = {};

		CommonMetadata::Deserialize(archive);

		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices);
		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices);

		check(NumVertices > 0);
		check(NumIndices > 0);
		return archive;
	}

	json& operator<<(json& archive, const StaticMeshImportConfig& config)
	{
		return config.Serialize(archive);
	}

	const json& operator>>(const json& archive, StaticMeshImportConfig& config)
	{
		return config.Deserialize(archive);
	}

	json& operator<<(json& archive, const StaticMeshLoadConfig& config)
	{
		return config.Serialize(archive);
	}

	const json& operator>>(const json& archive, StaticMeshLoadConfig& config)
	{
		return config.Deserialize(archive);
	}

	std::vector<xg::Guid> ModelImporter::ImportAsStatic(const String resPathStr, std::optional<StaticMeshImportConfig> importConfig /*= std::nullopt*/, const bool bIsPersistent /*= false*/)
	{
		/* #wip_todo 2024/03/12 여기서 부터 하기 */
		FE_LOG(ModelImporterInfo, "Importing resource {} as static mesh assets ...", resPathStr.AsStringView());

		const fs::path resPath{ resPathStr.AsStringView() };
		if (!fs::exists(resPath))
		{
			FE_LOG(ModelImporterErr, "The resource does not exist at {}.", resPathStr.AsStringView());
			return {};
		}

		const fs::path resMetaPath{ MakeResourceMetadataPath(resPath) };
		if (!importConfig)
		{
			importConfig = details::LoadMetadataFromFile<StaticMeshImportConfig, ModelImporterWarn>(resMetaPath);
		}

		const bool bPersistencyRequired = bIsPersistent || importConfig->bIsPersistent;
		bPersistencyRequired;

		// load static mesh import config
		// load model file using assimp
		// for mesh : meshes
		// create static mesh asset
		//
		unimplemented();
		return {};
	}

	std::optional<fe::StaticMesh> StaticMeshLoader::Load(const xg::Guid&, HandleManager&, RenderDevice&, GpuUploader&, GpuViewManager&)
	{
		unimplemented();
		return {};
	}

} // namespace fe