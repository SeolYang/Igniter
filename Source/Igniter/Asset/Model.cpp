#include <Igniter.h>
#include <Core/Timer.h>
#include <Core/Log.h>
#include <Core/JsonUtils.h>
#include <Core/Serializable.h>
#include <Filesystem/Utils.h>
#include <D3D12/RenderDevice.h>
#include <D3D12/GpuBuffer.h>
#include <D3D12/GpuBufferDesc.h>
#include <Render/GpuViewManager.h>
#include <Render/GpuUploader.h>
#include <Render/Vertex.h>
#include <Asset/Model.h>
#include <Asset/Texture.h>
#include <Asset/Utils.h>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(ModelImporterInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(ModelImporterWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(ModelImporterErr, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(ModelImporterFatal, ELogVerbosity::Fatal)
    IG_DEFINE_LOG_CATEGORY(StaticMeshLoaderInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(StaticMeshLoaderWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(StaticMeshLoaderErr, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(StaticMeshLoaderFatal, ELogVerbosity::Fatal)

    json& StaticMeshImportConfig::Serialize(json& archive) const
    {
        const auto& config = *this;
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMakeLeftHanded);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateNormals);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bSplitLargeMeshes);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bPreTransformVertices);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImproveCacheLocality);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUVCoords);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUVs);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipWindingOrder);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateBoundingBoxes);
        IG_SERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMergeMeshes);
        return archive;
    }

    const json& StaticMeshImportConfig::Deserialize(const json& archive)
    {
        auto& config = *this;
        config = {};
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMakeLeftHanded, config.bMakeLeftHanded);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipUVs, config.bFlipUVs);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bFlipWindingOrder, config.bFlipWindingOrder);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateNormals, config.bGenerateNormals);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bSplitLargeMeshes, config.bSplitLargeMeshes);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bPreTransformVertices, config.bPreTransformVertices);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImproveCacheLocality, config.bImproveCacheLocality);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateUVCoords, config.bGenerateUVCoords);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bGenerateBoundingBoxes, config.bGenerateBoundingBoxes);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bMergeMeshes, config.bMergeMeshes);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImportTextures, config.bImportTextures);
        IG_DESERIALIZE_JSON(StaticMeshImportConfig, archive, config, bImportMaterials, config.bImportMaterials);
        return archive;
    }

    json& StaticMeshLoadConfig::Serialize(json& archive) const
    {
        const auto& config = *this;
        IG_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, Name);
        IG_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices);
        IG_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices);
        IG_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedVerticesSizeInBytes);
        IG_SERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedIndicesSizeInBytes);
        return archive;
    }

    const json& StaticMeshLoadConfig::Deserialize(const json& archive)
    {
        auto& config = *this;
        config = {};
        IG_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, Name, config.Name);
        IG_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumVertices, config.NumVertices);
        IG_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, NumIndices, config.NumIndices);
        IG_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedVerticesSizeInBytes, config.CompressedVerticesSizeInBytes);
        IG_DESERIALIZE_JSON(StaticMeshLoadConfig, archive, config, CompressedIndicesSizeInBytes, config.CompressedIndicesSizeInBytes);
        return archive;
    }

    static bool CheckAssimpSceneLoadingSucceed(const String resPathStr, const Assimp::Importer& importer, const aiScene* scene)
    {
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            IG_LOG(ModelImporterErr, "Load model file from \"{}\" failed: \"{}\"", resPathStr.ToStringView(), importer.GetErrorString());
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

    static std::optional<xg::Guid> SaveStaticMeshAsset(const String resPathStr, const bool bIsPersistent, const std::string_view meshName, const std::vector<StaticMeshVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        const xg::Guid newGuid = xg::newGuid();
        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, newGuid);
        const std::string_view resPathStrView = resPathStr.ToStringView();
        const std::string assetPathStr = assetPath.string();

        std::vector<uint32_t> remap(indices.size());
        const size_t remappedVertexCount = meshopt_generateVertexRemap(remap.data(),
                                                                       indices.data(), indices.size(),
                                                                       vertices.data(), indices.size(), sizeof(StaticMeshVertex));

        std::vector<uint32_t> remappedIndices(indices.size());
        std::vector<StaticMeshVertex> remappedVertices(remappedVertexCount);

        meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
        meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(StaticMeshVertex), remap.data());
        IG_LOG(ModelImporterInfo, "Remapped #Vertices {} -> {}.", vertices.size(), remappedVertexCount);

        constexpr float CacheHitRatioThreshold = 1.02f;
        meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.size());
        meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex), CacheHitRatioThreshold);
        meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex));

        /* #sy_ref https://www.realtimerendering.com/blog/acmr-and-atvr/ */
        IG_LOG(ModelImporterInfo, "Optimization Statistics \"{}({}.{})\"", assetPathStr, resPathStrView, meshName);
        const auto nvidiaVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 32, 32, 32);
        const auto amdVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 14, 64, 128);
        const auto intelVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), 128, 0, 0);
        const auto vfs = meshopt_analyzeVertexFetch(remappedIndices.data(), remappedIndices.size(), remappedVertices.size(), sizeof(StaticMeshVertex));
        const auto os = meshopt_analyzeOverdraw(remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex));
        IG_LOG(ModelImporterInfo, "Vertex Cache Statistics(NVIDIA) - ACMR: {} / ATVR: {}", nvidiaVcs.acmr, nvidiaVcs.atvr);
        IG_LOG(ModelImporterInfo, "Vertex Cache Statistics(AMD) - ACMR: {} / ATVR: {}", amdVcs.acmr, amdVcs.atvr);
        IG_LOG(ModelImporterInfo, "Vertex Cache Statistics(INTEL) - ACMR: {} / ATVR: {}", intelVcs.acmr, intelVcs.atvr);
        IG_LOG(ModelImporterInfo, "Overfecth: {}", vfs.overfetch);
        IG_LOG(ModelImporterInfo, "Overdraw: {}", os.overdraw);

        /* Vertex/Index buffer compression */
        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        std::vector<uint8_t> encodedVertices(meshopt_encodeVertexBufferBound(remappedVertices.size(), sizeof(StaticMeshVertex)));
        std::vector<uint8_t> encodedIndices(meshopt_encodeIndexBufferBound(remappedIndices.size(), remappedVertices.size()));

        encodedVertices.resize(meshopt_encodeVertexBuffer(encodedVertices.data(), encodedVertices.size(), remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex)));
        encodedIndices.resize(meshopt_encodeIndexBuffer(encodedIndices.data(), encodedIndices.size(), remappedIndices.data(), remappedIndices.size()));

        const AssetInfo assetInfo{
            .CreationTime = Timer::Now(),
            .Guid = newGuid,
            .VirtualPath = meshName,
            .Type = EAssetType::StaticMesh,
            .bIsPersistent = bIsPersistent
        };

        const StaticMeshLoadConfig newLoadConfig{
            .NumVertices = static_cast<uint32_t>(remappedVertices.size()),
            .NumIndices = static_cast<uint32_t>(remappedIndices.size()),
            .CompressedVerticesSizeInBytes = encodedVertices.size(),
            .CompressedIndicesSizeInBytes = encodedIndices.size()
        };

        json assetMetadata{};
        assetMetadata << assetInfo << newLoadConfig;

        const fs::path newMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, assetInfo.Guid);
        if (!SaveJsonToFile(newMetaPath, assetMetadata))
        {
            IG_LOG(ModelImporterErr, "Failed to save asset metadata to {}.", newMetaPath.string());
            return std::nullopt;
        }

        if (!SaveBlobsToFile<2>(assetPath, { std::span<const uint8_t>{ encodedVertices }, std::span<const uint8_t>{ encodedIndices } }))
        {
            IG_LOG(ModelImporterErr, "Failed to save vertices/indices data to file {}.", assetPath.string());
            return std::nullopt;
        }

        return assetInfo.Guid;
    }

    std::vector<xg::Guid> ModelImporter::ImportAsStatic(TextureImporter& textureImporter, const String resPathStr, std::optional<StaticMeshImportConfig> importConfig /*= std::nullopt*/, const bool bIsPersistent /*= false*/)
    {
        TempTimer tempTimer;
        tempTimer.Begin();

        IG_LOG(ModelImporterInfo, "Importing resource {} as static mesh assets ...", resPathStr.ToStringView());
        const fs::path resPath{ resPathStr.ToStringView() };
        if (!fs::exists(resPath))
        {
            IG_LOG(ModelImporterErr, "The resource does not exist at {}.", resPathStr.ToStringView());
            return {};
        }

        const fs::path resMetaPath{ MakeResourceMetadataPath(resPath) };
        if (!importConfig)
        {
            json resMetadata{ LoadJsonFromFile(resMetaPath) };
            importConfig = StaticMeshImportConfig{};
            resMetadata >> *importConfig;
        }

        uint32_t importFlags = aiProcess_Triangulate;
        importFlags |= importConfig->bMakeLeftHanded ? aiProcess_MakeLeftHanded : 0;
        importFlags |= importConfig->bFlipUVs ? aiProcess_FlipUVs : 0;
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
        const aiScene* scene = importer.ReadFile(resPathStr.ToStandard(), importFlags);
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
                const fs::path texResPath = texResPathStr.ToStringView();

                /* #sy_wip 에셋 매니저 경유 */
                textureImporter;
                // if (!HasImportedBefore(texResPath))
                //{
                //     std::optional<xg::Guid> newTexAssetGuid = textureImporter.Import(texResPathStr);
                //     if (newTexAssetGuid)
                //     {
                //         IG_LOG(ModelImporterInfo, "Success to import texture resource {} as asset {}.", texResPathStr.AsStringView(), newTexAssetGuid->str());
                //     }
                //     else
                //     {
                //         IG_LOG(ModelImporterErr, "Failed to import texture resource {}.", texResPathStr.AsStringView());
                //     }
                // }
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

            if (std::optional<xg::Guid> guid = SaveStaticMeshAsset(resPathStr, bIsPersistent, meshName, vertices, indices);
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

                std::optional<xg::Guid> guid = SaveStaticMeshAsset(resPathStr, bIsPersistent, std::format("{}.{}", meshName, mesh.mName.C_Str()), vertices, indices);
                if (guid)
                {
                    importedStaticMeshGuid[meshIdx] = *guid;
                }
            }
        }
        importer.FreeScene();

        const ResourceInfo resInfo{ .Type = EAssetType::StaticMesh };
        json resMetadata{};
        resMetadata << resInfo << *importConfig;
        if (!SaveJsonToFile(resMetaPath, resMetadata))
        {
            IG_LOG(ModelImporterErr, "Failed to save resource metadata to {}.", resMetaPath.string());
        }

        IG_LOG(ModelImporterInfo, "Successfully imported resource {} as static mesh assets. Elapsed: {} ms", resPathStr.ToStringView(), tempTimer.End());
        return importedStaticMeshGuid;
    }

    std::optional<ig::StaticMesh> StaticMeshLoader::Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
    {
        IG_LOG(StaticMeshLoaderInfo, "Load static mesh {}.", guid.str());
        TempTimer tempTimer;
        tempTimer.Begin();

        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, guid);
        const fs::path assetMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, guid);
        if (!fs::exists(assetPath))
        {
            IG_LOG(StaticMeshLoaderFatal, "Static Mesh Asset \"{}\" does not exists.", assetPath.string());
            return std::nullopt;
        }

        if (!fs::exists(assetMetaPath))
        {
            IG_LOG(StaticMeshLoaderFatal, "Static Mesh Asset metadata \"{}\" does not exists.", assetMetaPath.string());
            return std::nullopt;
        }

        const json assetMetadata{ LoadJsonFromFile(assetMetaPath) };
        if (assetMetadata.empty())
        {
            IG_LOG(StaticMeshLoaderFatal, "Failed to load asset metadata from {}.", assetMetaPath.string());
            return std::nullopt;
        }

        AssetInfo assetInfo{};
        StaticMeshLoadConfig loadConfig{};
        assetMetadata >> loadConfig >> assetInfo;

        if (assetInfo.Guid != guid)
        {
            IG_LOG(StaticMeshLoaderFatal, "Asset guid does not match. Expected: {}, Found: {}",
                   guid.str(), assetInfo.Guid.str());
            return std::nullopt;
        }

        if (assetInfo.Type != EAssetType::StaticMesh)
        {
            IG_LOG(StaticMeshLoaderFatal, "Asset type does not match. Expected: {}, Found: {}",
                   magic_enum::enum_name(EAssetType::StaticMesh),
                   magic_enum::enum_name(assetInfo.Type));
            return std::nullopt;
        }

        if (loadConfig.NumVertices == 0 ||
            loadConfig.NumIndices == 0 ||
            loadConfig.CompressedVerticesSizeInBytes == 0 ||
            loadConfig.CompressedIndicesSizeInBytes == 0)
        {
            IG_LOG(StaticMeshLoaderFatal,
                   "Load config has invalid values. \n * NumVertices: {}\n * NumIndices: {}\n * CompressedVerticesSizeInBytes: {}\n * CompressedIndicesSizeInBytes: {}",
                   loadConfig.NumVertices, loadConfig.NumIndices, loadConfig.CompressedVerticesSizeInBytes, loadConfig.CompressedIndicesSizeInBytes);
            return std::nullopt;
        }

        std::vector<uint8_t> blob = LoadBlobFromFile(assetPath);
        if (blob.empty())
        {
            IG_LOG(StaticMeshLoaderFatal, "Static Mesh {} blob is empty.", assetPath.string());
            return std::nullopt;
        }

        const size_t expectedBlobSize = loadConfig.CompressedVerticesSizeInBytes + loadConfig.CompressedIndicesSizeInBytes;
        if (blob.size() != expectedBlobSize)
        {
            IG_LOG(StaticMeshLoaderFatal, "Static Mesh blob size does not match. Expected: {}, Found: {}", expectedBlobSize, blob.size());
            return std::nullopt;
        }

        GpuBufferDesc vertexBufferDesc{};
        vertexBufferDesc.AsStructuredBuffer<StaticMeshVertex>(loadConfig.NumVertices);
        vertexBufferDesc.DebugName = String(std::format("{}_Vertices", guid.str()));
        if (vertexBufferDesc.GetSizeAsBytes() < loadConfig.CompressedVerticesSizeInBytes)
        {
            IG_LOG(StaticMeshLoaderFatal, "Compressed vertices size exceed expected vertex buffer size. Compressed Size: {} bytes, Expected Vertex Buffer Size: {} bytes",
                   loadConfig.CompressedVerticesSizeInBytes,
                   vertexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        GpuBufferDesc indexBufferDesc{};
        indexBufferDesc.AsIndexBuffer<uint32_t>(loadConfig.NumIndices);
        indexBufferDesc.DebugName = String(std::format("{}_Indices", guid.str()));
        if (indexBufferDesc.GetSizeAsBytes() < loadConfig.CompressedIndicesSizeInBytes)
        {
            IG_LOG(StaticMeshLoaderFatal, "Compressed indices size exceed expected index buffer size. Compressed Size: {} bytes, Expected Index Buffer Size: {} bytes",
                   loadConfig.CompressedIndicesSizeInBytes,
                   indexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        std::optional<GpuBuffer> vertexBuffer = renderDevice.CreateBuffer(vertexBufferDesc);
        if (!vertexBuffer)
        {
            IG_LOG(StaticMeshLoaderFatal, "Failed to create vertex buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        auto vertexBufferSrv = gpuViewManager.RequestShaderResourceView(*vertexBuffer);
        if (!vertexBufferSrv)
        {
            IG_LOG(StaticMeshLoaderFatal, "Failed to create vertex buffer shader resource view of {}.", assetPath.string());
            return std::nullopt;
        }

        std::optional<GpuBuffer> indexBuffer = renderDevice.CreateBuffer(indexBufferDesc);
        if (!indexBuffer)
        {
            IG_LOG(StaticMeshLoaderFatal, "Failed to create index buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadConfig.NumVertices,
                                                                sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, loadConfig.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBuffer);
            }
            else
            {
                IG_LOG(StaticMeshLoaderErr, "Failed to decode vertex buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        UploadContext indicesUploadCtx = gpuUploader.Reserve(indexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedIndicesOffset = loadConfig.CompressedVerticesSizeInBytes;
            const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(
                reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                loadConfig.NumIndices,
                blob.data() + compressedIndicesOffset, loadConfig.CompressedIndicesSizeInBytes);

            if (decodeResult == 0)
            {
                indicesUploadCtx.CopyBuffer(0, indexBufferDesc.GetSizeAsBytes(), *indexBuffer);
            }
            else
            {
                IG_LOG(StaticMeshLoaderErr, "Failed to decode index buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

        verticesUploadSync->WaitOnCpu();
        indicesUploadSync->WaitOnCpu();

        IG_LOG(StaticMeshLoaderInfo,
               "Successfully load static mesh asset {}, which from resource {}. Elapsed: {} ms",
               assetPath.string(), assetInfo.VirtualPath.ToStringView(), tempTimer.End());

        return StaticMesh{
            .LoadConfig = loadConfig,
            .VertexBufferInstance = { handleManager, std::move(*vertexBuffer) },
            .VertexBufferSrv = std::move(vertexBufferSrv),
            .IndexBufferInstance = { handleManager, std::move(*indexBuffer) }
        };
    }
} // namespace ig