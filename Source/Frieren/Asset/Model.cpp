#include <Asset/Model.h>
#include <Asset/Texture.h>
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
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUVCoords);
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUVs);
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
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUVCoords);
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUVs);
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

	static bool CheckAssimpSceneLoadingSucceed(const String resPathStr, const Assimp::Importer& importer, const aiScene* scene)
	{
		if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
		{
			FE_LOG(ModelImporterErr, "Load model file from \"{}\" failed: \"{}\"", resPathStr.AsStringView(), importer.GetErrorString());
			return false;
		}

		return true;
	}

	std::vector<xg::Guid> ModelImporter::ImportAsStatic(TextureImporter& textureImporter, const String resPathStr, std::optional<StaticMeshImportConfig> importConfig /*= std::nullopt*/, const bool bIsPersistent /*= false*/)
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
			if (!importConfig)
			{
				importConfig = MakeVersionedDefault<StaticMeshImportConfig>();
			}
		}

		const bool bPersistencyRequired = bIsPersistent || importConfig->bIsPersistent;

		uint32_t importFlags = aiProcess_Triangulate;
		importFlags |= importConfig->bMakeLeftHanded ? aiProcess_MakeLeftHanded : 0;
		importFlags |= importConfig->bGenerateNormals ? aiProcess_GenSmoothNormals : 0;
		importFlags |= importConfig->bSplitLargeMeshes ? aiProcess_SplitLargeMeshes : 0;
		importFlags |= importConfig->bPreTransformVertices ? aiProcess_PreTransformVertices : 0;
		importFlags |= importConfig->bGenerateUVCoords ? aiProcess_GenUVCoords : 0;
		importFlags |= importConfig->bFlipUVs ? aiProcess_FlipUVs : 0;
		importFlags |= importConfig->bFlipWindingOrder ? aiProcess_FlipWindingOrder : 0;
		importFlags |= importConfig->bGenerateBoundingBoxes ? aiProcess_GenBoundingBoxes : 0;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(resPathStr.AsString(), importFlags);
		if (!CheckAssimpSceneLoadingSucceed(resPathStr, importer, scene))
		{
			return {};
		}

		/* Import Textures */
		/* #todo Impl Import Textures in aiScene */
		if (importConfig->bImportTextures)
		{
			for (size_t idx = 0; idx < scene->mNumTextures; ++idx)
			{
				const aiTexture& texture = *scene->mTextures[idx];
				const String texResPathStr = String(texture.mFilename.C_Str());
				const fs::path texResPath = texResPathStr.AsStringView();

				if (!HasImportedBefore(texResPath))
				{
					std::optional<xg::Guid> newTexAssetGuid = textureImporter.Import(texResPathStr, MakeVersionedDefault<TextureImportConfig>());
					if (newTexAssetGuid)
					{
						FE_LOG(ModelImporterInfo, "Success to import texture resource {} as asset {}.", texResPathStr.AsStringView(), newTexAssetGuid->str());
					}
					else
					{
						FE_LOG(ModelImporterErr, "Failed to import texture resource {}.", texResPathStr.AsStringView());
					}
				}
			}
		}

		/* Import Materials */
		/* #todo Impl Import Materials in aiScene */

		/* Import Static Meshes */
		for (size_t idx = 0; idx < scene->mNumMeshes; ++idx)
		{
			constexpr size_t NumIndicesPerFace = 3;
			const aiMesh& mesh = *scene->mMeshes[idx];
			std::vector<StaticMeshVertex> vertices(mesh.mNumVertices);
			std::vector<uint32_t> indices(mesh.mNumFaces * NumIndicesPerFace);

			for (size_t vertexIdx = 0; vertexIdx < mesh.mNumVertices; ++vertexIdx)
			{
				const aiVector3D& position = mesh.mVertices[vertexIdx];
				const aiVector3D& normal = mesh.mNormals[vertexIdx];
				const aiVector3D& uvCoords = mesh.mTextureCoords[0][vertexIdx];

				vertices[vertexIdx] = StaticMeshVertex{
					.Position = Vector3{ position.x, position.y, position.z },
					.Normal = Vector3{ normal.x, normal.y, normal.z },
					.UVCoords = Vector2{ uvCoords.x, uvCoords.y }
				};
			}

			for (size_t faceIdx = 0; faceIdx < mesh.mNumFaces; ++faceIdx)
			{
				const aiFace& face = mesh.mFaces[faceIdx];
				check(face.mNumIndices == NumIndicesPerFace);
				indices[faceIdx * NumIndicesPerFace + 0] = face.mIndices[0];
				indices[faceIdx * NumIndicesPerFace + 1] = face.mIndices[1];
				indices[faceIdx * NumIndicesPerFace + 2] = face.mIndices[2];
			}

			auto newStaticMeshLoadConf = MakeVersionedDefault<StaticMeshLoadConfig>();
			newStaticMeshLoadConf.bIsPersistent = bPersistencyRequired;
			newStaticMeshLoadConf.NumVertices = vertices.size();
			newStaticMeshLoadConf.NumIndices = indices.size();
		}

		unimplemented();
		return {};
	}

	std::optional<fe::StaticMesh> StaticMeshLoader::Load(const xg::Guid&, HandleManager&, RenderDevice&, GpuUploader&, GpuViewManager&)
	{
		unimplemented();
		return {};
	}

} // namespace fe