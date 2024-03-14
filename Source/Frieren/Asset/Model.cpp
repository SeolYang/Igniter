#include <Asset/Model.h>
#include <Asset/Texture.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/GpuViewManager.h>
#include <Render/GpuUploader.h>
#include <Render/Vertex.h>
#include <Core/Timer.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>

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
		FE_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMergeMeshes);

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
		FE_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMergeMeshes);

		return archive;
	}

	json& StaticMeshLoadConfig::Serialize(json& archive) const
	{
		check(NumVertices > 0);
		check(NumIndices > 0);
		check(CompressedVerticesSizeInBytes > 0);
		check(CompressedIndicesSizeInBytes > 0);

		CommonMetadata::Serialize(archive);

		const auto& config = *this;
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, Name);
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices);
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices);
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedVerticesSizeInBytes);
		FE_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedIndicesSizeInBytes);

		return archive;
	}

	const json& StaticMeshLoadConfig::Deserialize(const json& archive)
	{
		auto& config = *this;
		config = {};

		CommonMetadata::Deserialize(archive);

		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, Name);
		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices);
		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices);
		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedVerticesSizeInBytes);
		FE_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedIndicesSizeInBytes);

		check(NumVertices > 0);
		check(NumIndices > 0);
		check(CompressedVerticesSizeInBytes > 0);
		check(CompressedIndicesSizeInBytes > 0);
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

	static void ProcessStaticMeshData(const aiMesh& mesh, std::vector<StaticMeshVertex>& vertices, std::vector<uint32_t>& indices, const uint32_t vertexIdxOffset = 0)
	{
		for (size_t vertexIdx = 0; vertexIdx < mesh.mNumVertices; ++vertexIdx)
		{
			const aiVector3D& position = mesh.mVertices[vertexIdx];
			const aiVector3D& normal = mesh.mNormals[vertexIdx];
			const aiVector3D& uvCoords = mesh.mTextureCoords[0][vertexIdx];

			vertices.emplace_back(
				Vector3{ position.x, position.y, position.z },
				Vector3{ normal.x, normal.y, normal.z },
				Vector2{ uvCoords.x, uvCoords.y });
		}

		for (size_t faceIdx = 0; faceIdx < mesh.mNumFaces; ++faceIdx)
		{
			const aiFace& face = mesh.mFaces[faceIdx];
			indices.emplace_back(vertexIdxOffset + face.mIndices[0]);
			indices.emplace_back(vertexIdxOffset + face.mIndices[1]);
			indices.emplace_back(vertexIdxOffset + face.mIndices[2]);
		}
	}

	static std::optional<xg::Guid> SaveStaticMeshAsset(const String resPathStr, const bool bPersistencyRequired, const std::string_view meshName, const std::vector<StaticMeshVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (!fs::exists(details::StaticMeshAssetRootPath))
		{
			details::CreateAssetDirectories();
		}

		const xg::Guid newGuid = xg::newGuid();
		const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, newGuid);
		const std::string_view resPathStrView = resPathStr.AsStringView();
		const std::string assetPathStr = assetPath.string();

		std::vector<uint32_t> remap(indices.size());
		const size_t remappedVertexCount = meshopt_generateVertexRemap(remap.data(),
																	   indices.data(), indices.size(),
																	   vertices.data(), indices.size(), sizeof(StaticMeshVertex));

		std::vector<uint32_t> remappedIndices(indices.size());
		std::vector<StaticMeshVertex> remappedVertices(remappedVertexCount);

		meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
		meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(StaticMeshVertex), remap.data());
		FE_LOG(ModelImporterInfo, "Remapped #Vertices {} -> {}.", vertices.size(), remappedVertexCount);

		constexpr float CacheHitRatioThreshold = 1.02f;
		meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.size());
		meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex), CacheHitRatioThreshold);
		meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex));

		/* #sy_ref https://www.realtimerendering.com/blog/acmr-and-atvr/ */
		FE_LOG(ModelImporterInfo, "Optimization Statistics \"{}({}.{})\"", assetPathStr, resPathStrView, meshName);
		const auto nvidiaVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 32, 32, 32);
		const auto amdVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 14, 64, 128);
		const auto intelVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 128, 0, 0);
		const auto vfs = meshopt_analyzeVertexFetch(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), sizeof(StaticMeshVertex));
		const auto os = meshopt_analyzeOverdraw(remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex));
		FE_LOG(ModelImporterInfo, "Vertex Cache Statistics(NVIDIA) - ACMR: {} / ATVR: {}", nvidiaVcs.acmr, nvidiaVcs.atvr);
		FE_LOG(ModelImporterInfo, "Vertex Cache Statistics(AMD) - ACMR: {} / ATVR: {}", amdVcs.acmr, amdVcs.atvr);
		FE_LOG(ModelImporterInfo, "Vertex Cache Statistics(INTEL) - ACMR: {} / ATVR: {}", intelVcs.acmr, intelVcs.atvr);
		FE_LOG(ModelImporterInfo, "Overfecth: {}", vfs.overfetch);
		FE_LOG(ModelImporterInfo, "Overdraw: {}", os.overdraw);

		/* Vertex/Index buffer compression */
		meshopt_encodeVertexVersion(0);
		meshopt_encodeIndexVersion(1);

		std::vector<uint8_t> encodedVertices(meshopt_encodeVertexBufferBound(remappedVertices.size(), sizeof(StaticMeshVertex)));
		std::vector<uint8_t> encodedIndices(meshopt_encodeIndexBufferBound(remappedIndices.size(), remappedVertices.size()));

		encodedVertices.resize(meshopt_encodeVertexBuffer(encodedVertices.data(), encodedVertices.size(), remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex)));
		encodedIndices.resize(meshopt_encodeIndexBuffer(encodedIndices.data(), encodedIndices.size(), remappedIndices.data(), remappedIndices.size()));

		auto newStaticMeshLoadConf = MakeVersionedDefault<StaticMeshLoadConfig>();
		newStaticMeshLoadConf.CreationTime = Timer::Now();
		newStaticMeshLoadConf.Guid = newGuid;
		newStaticMeshLoadConf.SrcResPath = resPathStr;
		newStaticMeshLoadConf.Type = EAssetType::StaticMesh;
		newStaticMeshLoadConf.bIsPersistent = bPersistencyRequired;
		newStaticMeshLoadConf.Name = meshName;
		newStaticMeshLoadConf.NumVertices = static_cast<uint32_t>(remappedVertices.size());
		newStaticMeshLoadConf.NumIndices = static_cast<uint32_t>(remappedIndices.size());
		newStaticMeshLoadConf.CompressedVerticesSizeInBytes = encodedVertices.size();
		newStaticMeshLoadConf.CompressedIndicesSizeInBytes = encodedIndices.size();

		const fs::path newMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, newStaticMeshLoadConf.Guid);
		if (!SaveSerializedDataToJsonFile(newMetaPath, newStaticMeshLoadConf))
		{
			FE_LOG(ModelImporterErr, "Failed to save asset metadata to {}.", newMetaPath.string());
			return std::nullopt;
		}

		if (!SaveBlobsToFile<2>(assetPath, { std::span<const uint8_t>{ encodedVertices }, std::span<const uint8_t>{ encodedIndices } }))
		{
			FE_LOG(ModelImporterErr, "Failed to save vertices/indices data to file {}.", assetPath.string());
			return std::nullopt;
		}

		return newStaticMeshLoadConf.Guid;
	}

	std::vector<xg::Guid> ModelImporter::ImportAsStatic(TextureImporter& textureImporter, const String resPathStr, std::optional<StaticMeshImportConfig> importConfig /*= std::nullopt*/, const bool bIsPersistent /*= false*/)
	{
		TempTimer tempTimer;
		tempTimer.Begin();

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
		importFlags |= importConfig->bFlipUVs ?  aiProcess_FlipUVs : 0;
		importFlags |= importConfig->bFlipWindingOrder ? aiProcess_FlipWindingOrder : 0;

		if (importConfig->bMergeMeshes)
		{
			importFlags |= aiProcess_PreTransformVertices;
		}
		else
		{
			importFlags |= importConfig->bSplitLargeMeshes ? aiProcess_SplitLargeMeshes : 0;
			importFlags |= importConfig->bPreTransformVertices ? aiProcess_PreTransformVertices : 0;
		}

		importFlags |= importConfig->bGenerateNormals ? aiProcess_GenSmoothNormals : 0;
		importFlags |= importConfig->bGenerateUVCoords ? aiProcess_GenUVCoords : 0;
		importFlags |= importConfig->bGenerateBoundingBoxes ? aiProcess_GenBoundingBoxes : 0;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(resPathStr.AsString(), importFlags);
		if (!CheckAssimpSceneLoadingSucceed(resPathStr, importer, scene))
		{
			return {};
		}

		/* Import Textures */
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
		/* #sy_todo Impl Import Materials in aiScene */

		/* Import Static Meshes */
		const std::string meshName = resPath.filename().replace_extension().string();

		constexpr size_t NumIndicesPerFace = 3;
		std::vector<xg::Guid> importedStaticMeshGuid(importConfig->bMergeMeshes ? 1 : scene->mNumMeshes);
		if (importConfig->bMergeMeshes)
		{
			std::vector<StaticMeshVertex> vertices;
			std::vector<uint32_t> indices;

			for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
			{
				const aiMesh& mesh = *scene->mMeshes[meshIdx];
				vertices.reserve(vertices.size() + mesh.mNumVertices);
				indices.reserve(indices.size() + mesh.mNumFaces * NumIndicesPerFace);
				ProcessStaticMeshData(mesh, vertices, indices, static_cast<uint32_t>(vertices.size()));
			}

			if (std::optional<xg::Guid> guid = SaveStaticMeshAsset(resPathStr, bPersistencyRequired, meshName, vertices, indices);
				guid)
			{
				importedStaticMeshGuid[0] = *guid;
			}
		}
		else
		{
			for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
			{
				const aiMesh& mesh = *scene->mMeshes[meshIdx];
				std::vector<StaticMeshVertex> vertices;
				std::vector<uint32_t> indices;

				vertices.reserve(mesh.mNumVertices);
				indices.reserve(mesh.mNumFaces * NumIndicesPerFace);

				ProcessStaticMeshData(mesh, vertices, indices);

				std::optional<xg::Guid> guid = SaveStaticMeshAsset(resPathStr, bPersistencyRequired, std::format("{}.{}", meshName, mesh.mName.C_Str()), vertices, indices);
				if (guid)
				{
					importedStaticMeshGuid[meshIdx] = *guid;
				}
			}
		}
		importer.FreeScene();

		importConfig->Version = StaticMeshImportConfig::LatestVersion;
		importConfig->AssetType = EAssetType::StaticMesh;
		importConfig->bIsPersistent = bPersistencyRequired;
		if (!SaveSerializedDataToJsonFile(resMetaPath, *importConfig))
		{
			FE_LOG(ModelImporterErr, "Failed to save resource metadata to {}.", resMetaPath.string());
		}

		FE_LOG(ModelImporterInfo, "Successfully imported resource {} as static mesh assets. Elapsed: {} ms", resPathStr.AsStringView(), tempTimer.End());
		return importedStaticMeshGuid;
	}

	std::optional<fe::StaticMesh> StaticMeshLoader::Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
	{
		FE_LOG(StaticMeshLoaderInfo, "Load static mesh {}.", guid.str());
		TempTimer tempTimer;
		tempTimer.Begin();

		const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, guid);
		const fs::path assetMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, guid);
		if (!fs::exists(assetPath))
		{
			FE_LOG(StaticMeshLoaderFatal, "Static Mesh Asset \"{}\" does not exists.", assetPath.string());
			return std::nullopt;
		}

		if (!fs::exists(assetMetaPath))
		{
			FE_LOG(StaticMeshLoaderFatal, "Static Mesh Asset metadata \"{}\" does not exists.", assetMetaPath.string());
			return std::nullopt;
		}

		std::optional<StaticMeshLoadConfig> loadConfig = LoadSerializedDataFromJsonFile<StaticMeshLoadConfig>(assetMetaPath);
		if (!loadConfig)
		{
			FE_LOG(StaticMeshLoaderFatal, "Failed to read Static Mesh Load Config from file {}.", assetMetaPath.string());
			return std::nullopt;
		}

		if (!loadConfig->IsLatestVersion())
		{
			details::LogVersionMismatch<StaticMeshLoadConfig, StaticMeshLoaderWarn>(*loadConfig, assetMetaPath.string());
		}

		if (loadConfig->Guid != guid)
		{
			FE_LOG(StaticMeshLoaderFatal, "Asset guid does not match. Expected: {}, Found: {}",
				   guid.str(), loadConfig->Guid.str());
			return std::nullopt;
		}

		if (loadConfig->Type != EAssetType::StaticMesh)
		{
			FE_LOG(StaticMeshLoaderFatal, "Asset type does not match. Expected: {}, Found: {}",
				   magic_enum::enum_name(EAssetType::StaticMesh),
				   magic_enum::enum_name(loadConfig->Type));
			return std::nullopt;
		}

		/* #sy_todo Impl ToString */
		if (loadConfig->NumVertices == 0 ||
			loadConfig->NumIndices == 0 ||
			loadConfig->CompressedVerticesSizeInBytes == 0 ||
			loadConfig->CompressedIndicesSizeInBytes == 0)
		{
			FE_LOG(StaticMeshLoaderFatal,
				   "Load config has invalid values. \n * NumVertices: {}\n * NumIndices: {}\n * CompressedVerticesSizeInBytes: {}\n * CompressedIndicesSizeInBytes: {}",
				   loadConfig->NumVertices, loadConfig->NumIndices, loadConfig->CompressedVerticesSizeInBytes, loadConfig->CompressedIndicesSizeInBytes);
			return std::nullopt;
		}

		std::vector<uint8_t> blob = LoadBlobFromFile(assetPath);
		if (blob.empty())
		{
			FE_LOG(StaticMeshLoaderFatal, "Static Mesh {} blob is empty.", assetPath.string());
			return std::nullopt;
		}

		const size_t expectedBlobSize = loadConfig->CompressedVerticesSizeInBytes + loadConfig->CompressedIndicesSizeInBytes;
		if (blob.size() != expectedBlobSize)
		{
			FE_LOG(StaticMeshLoaderFatal, "Static Mesh blob size does not match. Expected: {}, Found: {}", expectedBlobSize, blob.size());
			return std::nullopt;
		}

		GpuBufferDesc vertexBufferDesc{};
		vertexBufferDesc.AsStructuredBuffer<StaticMeshVertex>(loadConfig->NumVertices);
		vertexBufferDesc.DebugName = String(std::format("{}_Vertices", guid.str()));
		if (vertexBufferDesc.GetSizeAsBytes() < loadConfig->CompressedVerticesSizeInBytes)
		{
			FE_LOG(StaticMeshLoaderFatal, "Compressed vertices size exceed expected vertex buffer size. Compressed Size: {} bytes, Expected Vertex Buffer Size: {} bytes",
				   loadConfig->CompressedVerticesSizeInBytes,
				   vertexBufferDesc.GetSizeAsBytes());
			return std::nullopt;
		}

		GpuBufferDesc indexBufferDesc{};
		indexBufferDesc.AsIndexBuffer<uint32_t>(loadConfig->NumIndices);
		indexBufferDesc.DebugName = String(std::format("{}_Indices", guid.str()));
		if (indexBufferDesc.GetSizeAsBytes() < loadConfig->CompressedIndicesSizeInBytes)
		{
			FE_LOG(StaticMeshLoaderFatal, "Compressed indices size exceed expected index buffer size. Compressed Size: {} bytes, Expected Index Buffer Size: {} bytes",
				   loadConfig->CompressedIndicesSizeInBytes,
				   indexBufferDesc.GetSizeAsBytes());
			return std::nullopt;
		}

		std::optional<GpuBuffer> vertexBuffer = renderDevice.CreateBuffer(vertexBufferDesc);
		if (!vertexBuffer)
		{
			FE_LOG(StaticMeshLoaderFatal, "Failed to create vertex buffer of {}.", assetPath.string());
			return std::nullopt;
		}

		auto vertexBufferSrv = gpuViewManager.RequestShaderResourceView(*vertexBuffer);
		if (!vertexBufferSrv)
		{
			FE_LOG(StaticMeshLoaderFatal, "Failed to create vertex buffer shader resource view of {}.", assetPath.string());
			return std::nullopt;
		}

		std::optional<GpuBuffer> indexBuffer = renderDevice.CreateBuffer(indexBufferDesc);
		if (!indexBuffer)
		{
			FE_LOG(StaticMeshLoaderFatal, "Failed to create index buffer of {}.", assetPath.string());
			return std::nullopt;
		}

		UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
		{
			const size_t compressedVerticesOffset = 0;
			const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadConfig->NumVertices,
																sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, loadConfig->CompressedVerticesSizeInBytes);
			if (decodeResult == 0)
			{
				verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBuffer);
			}
			else
			{
				FE_LOG(StaticMeshLoaderErr, "Failed to decode vertex buffer of {}.", assetPath.string());
			}
		}
		std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
		check(verticesUploadSync);

		UploadContext indicesUploadCtx = gpuUploader.Reserve(indexBufferDesc.GetSizeAsBytes());
		{
			const size_t compressedIndicesOffset = loadConfig->CompressedVerticesSizeInBytes;
			const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(
				reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
				loadConfig->NumIndices,
				blob.data() + compressedIndicesOffset, loadConfig->CompressedIndicesSizeInBytes);

			if (decodeResult == 0)
			{
				indicesUploadCtx.CopyBuffer(0, indexBufferDesc.GetSizeAsBytes(), *indexBuffer);
			}
			else
			{
				FE_LOG(StaticMeshLoaderErr, "Failed to decode index buffer of {}.", assetPath.string());
			}
		}
		std::optional<GpuSync> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
		check(indicesUploadSync);

		verticesUploadSync->WaitOnCpu();
		indicesUploadSync->WaitOnCpu();

		FE_LOG(StaticMeshLoaderInfo,
			   "Successfully load static mesh asset {}, which from resource {}. Elapsed: {} ms",
			   assetPath.string(), loadConfig->SrcResPath, tempTimer.End());

		return StaticMesh{
			.LoadConfig = *loadConfig,
			.VertexBufferInstance = { handleManager, std::move(*vertexBuffer) },
			.VertexBufferSrv = std::move(vertexBufferSrv),
			.IndexBufferInstance = { handleManager, std::move(*indexBuffer) }
		};
	}
} // namespace fe