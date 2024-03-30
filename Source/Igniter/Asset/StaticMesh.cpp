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
#include <Asset/StaticMesh.h>
#include <Asset/Texture.h>
#include <Asset/Utils.h>

IG_DEFINE_LOG_CATEGORY(ModelImporter);
IG_DEFINE_LOG_CATEGORY(StaticMeshLoader);

namespace ig
{
    constexpr inline size_t NumIndicesPerFace = 3;

    json& StaticMeshImportDesc::Serialize(json& archive) const
    {
        const auto& config = *this;
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bMakeLeftHanded);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateNormals);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bSplitLargeMeshes);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bPreTransformVertices);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bImproveCacheLocality);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateUVCoords);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bFlipUVs);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bFlipWindingOrder);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateBoundingBoxes);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, config, bImportMaterials);
        return archive;
    }

    const json& StaticMeshImportDesc::Deserialize(const json& archive)
    {
        auto& config = *this;
        config = {};
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bMakeLeftHanded, config.bMakeLeftHanded);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bFlipUVs, config.bFlipUVs);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bFlipWindingOrder, config.bFlipWindingOrder);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateNormals, config.bGenerateNormals);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bSplitLargeMeshes, config.bSplitLargeMeshes);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bPreTransformVertices, config.bPreTransformVertices);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bImproveCacheLocality, config.bImproveCacheLocality);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateUVCoords, config.bGenerateUVCoords);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bGenerateBoundingBoxes, config.bGenerateBoundingBoxes);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, config, bImportMaterials, config.bImportMaterials);
        return archive;
    }

    json& StaticMeshLoadDesc::Serialize(json& archive) const
    {
        const auto& config = *this;
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, config, NumVertices);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, config, NumIndices);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, config, CompressedVerticesSizeInBytes);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, config, CompressedIndicesSizeInBytes);
        return archive;
    }

    const json& StaticMeshLoadDesc::Deserialize(const json& archive)
    {
        auto& config = *this;
        config = {};
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, config, NumVertices, config.NumVertices);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, config, NumIndices, config.NumIndices);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, config, CompressedVerticesSizeInBytes, config.CompressedVerticesSizeInBytes);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, config, CompressedIndicesSizeInBytes, config.CompressedIndicesSizeInBytes);
        return archive;
    }

    static bool CheckAssimpSceneLoadingSucceed(const String resPathStr, const Assimp::Importer& importer, const aiScene* scene)
    {
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            IG_LOG(ModelImporter, Error, "Load model file from \"{}\" failed: \"{}\"", resPathStr.ToStringView(), importer.GetErrorString());
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
            IG_CHECK(face.mNumIndices == NumIndicesPerFace);
            indices.emplace_back(vertexIdxOffset + face.mIndices[0]);
            indices.emplace_back(vertexIdxOffset + face.mIndices[1]);
            indices.emplace_back(vertexIdxOffset + face.mIndices[2]);
        }
    }

    static Result<StaticMesh::Desc, EStaticMeshImportStatus> SaveStaticMeshAsset(const String resPathStr, const std::string_view meshName, const std::vector<StaticMeshVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        IG_CHECK(!vertices.empty());
        IG_CHECK(!indices.empty());
        IG_CHECK(resPathStr);

        const std::string_view resPathStrView = resPathStr.ToStringView();

        std::vector<uint32_t> remap(indices.size());
        const size_t remappedVertexCount = meshopt_generateVertexRemap(remap.data(),
                                                                       indices.data(), indices.size(),
                                                                       vertices.data(), indices.size(), sizeof(StaticMeshVertex));

        std::vector<uint32_t> remappedIndices(indices.size());
        std::vector<StaticMeshVertex> remappedVertices(remappedVertexCount);

        meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
        meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(StaticMeshVertex), remap.data());
        IG_LOG(ModelImporter, Info, "Remapped #Vertices {} -> {}.", vertices.size(), remappedVertexCount);

        constexpr float CacheHitRatioThreshold = 1.02f;
        meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.size());
        meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex), CacheHitRatioThreshold);
        meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(), remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex));

        /* #sy_ref https://www.realtimerendering.com/blog/acmr-and-atvr/ */
        IG_LOG(ModelImporter, Info, "Optimization Statistics \"{}({})\"", resPathStrView, meshName);
        const auto nvidiaVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(),
                                                          remappedVertices.size(),
                                                          32, 32, 32);

        const auto amdVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(),
                                                       remappedVertices.size(),
                                                       14, 64, 128);

        const auto intelVcs = meshopt_analyzeVertexCache(remappedIndices.data(), remappedIndices.size(),
                                                         remappedVertices.size(),
                                                         128, 0, 0);

        const auto vfs = meshopt_analyzeVertexFetch(remappedIndices.data(), remappedIndices.size(),
                                                    remappedVertices.size(),
                                                    sizeof(StaticMeshVertex));

        const auto os = meshopt_analyzeOverdraw(remappedIndices.data(), remappedIndices.size(), &remappedVertices[0].Position.x,
                                                remappedVertices.size(), sizeof(StaticMeshVertex));

        IG_LOG(ModelImporter, Info, "Vertex Cache Statistics(NVIDIA) - ACMR: {} / ATVR: {}", nvidiaVcs.acmr, nvidiaVcs.atvr);
        IG_LOG(ModelImporter, Info, "Vertex Cache Statistics(AMD) - ACMR: {} / ATVR: {}", amdVcs.acmr, amdVcs.atvr);
        IG_LOG(ModelImporter, Info, "Vertex Cache Statistics(INTEL) - ACMR: {} / ATVR: {}", intelVcs.acmr, intelVcs.atvr);
        IG_LOG(ModelImporter, Info, "Overfecth: {}", vfs.overfetch);
        IG_LOG(ModelImporter, Info, "Overdraw: {}", os.overdraw);

        /* Vertex/Index buffer compression */
        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        std::vector<uint8_t> encodedVertices(meshopt_encodeVertexBufferBound(remappedVertices.size(), sizeof(StaticMeshVertex)));
        std::vector<uint8_t> encodedIndices(meshopt_encodeIndexBufferBound(remappedIndices.size(), remappedVertices.size()));

        encodedVertices.resize(meshopt_encodeVertexBuffer(encodedVertices.data(), encodedVertices.size(),
                                                          remappedVertices.data(), remappedVertices.size(),
                                                          sizeof(StaticMeshVertex)));
        encodedIndices.resize(meshopt_encodeIndexBuffer(encodedIndices.data(), encodedIndices.size(),
                                                        remappedIndices.data(), remappedIndices.size()));

        const AssetInfo assetInfo{
            .CreationTime = Timer::Now(),
            .Guid = xg::newGuid(),
            .VirtualPath = meshName,
            .Type = EAssetType::StaticMesh
        };

        const StaticMeshLoadDesc newLoadConfig{
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
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveMetadataToFile>();
        }

        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, assetInfo.Guid);
        if (!SaveBlobsToFile<2>(assetPath, { std::span<const uint8_t>{ encodedVertices }, std::span<const uint8_t>{ encodedIndices } }))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid());
        IG_CHECK(newLoadConfig.NumVertices > 0 && newLoadConfig.NumIndices > 0);
        IG_CHECK(newLoadConfig.CompressedVerticesSizeInBytes > 0 && newLoadConfig.CompressedIndicesSizeInBytes);
        return MakeSuccess<StaticMesh::Desc, EStaticMeshImportStatus>(assetInfo, newLoadConfig);
    }

    std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> StaticMeshImporter::ImportStaticMesh(AssetManager& /*assetManager*/, const String resPathStr, StaticMesh::ImportDesc desc)
    {
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results;
        const fs::path resPath{ resPathStr.ToStringView() };
        if (!fs::exists(resPath))
        {
            results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FileDoesNotExist>());
            return results;
        }

        uint32_t importFlags = aiProcess_Triangulate;
        importFlags |= desc.bMakeLeftHanded ? aiProcess_MakeLeftHanded : 0;
        importFlags |= desc.bFlipUVs ? aiProcess_FlipUVs : 0;
        importFlags |= desc.bFlipWindingOrder ? aiProcess_FlipWindingOrder : 0;
        importFlags |= desc.bSplitLargeMeshes ? aiProcess_SplitLargeMeshes : 0;
        importFlags |= desc.bPreTransformVertices ? aiProcess_PreTransformVertices : 0;
        importFlags |= desc.bGenerateNormals ? aiProcess_GenSmoothNormals : 0;
        importFlags |= desc.bGenerateUVCoords ? aiProcess_GenUVCoords : 0;
        importFlags |= desc.bGenerateBoundingBoxes ? aiProcess_GenBoundingBoxes : 0;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(resPathStr.ToStandard(), importFlags);
        {
            if (!CheckAssimpSceneLoadingSucceed(resPathStr, importer, scene))
            {
                results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedLoadFromFile>());
                return results;
            }

            /* Create Materials */
            /* #sy_todo Impl Create Materials in aiScene */

            /* Import Static Meshes */
            const std::string modelName = resPath.filename().replace_extension().string();
            results.reserve(scene->mNumMeshes);
            std::vector<xg::Guid> importedStaticMeshGuid(scene->mNumMeshes);
            for (size_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
            {
                const aiMesh& mesh = *scene->mMeshes[meshIdx];
                std::vector<StaticMeshVertex> vertices;
                std::vector<uint32_t> indices;

                vertices.reserve(mesh.mNumVertices);
                indices.reserve(mesh.mNumFaces * NumIndicesPerFace);

                ProcessStaticMeshData(mesh, vertices, indices);

                if (vertices.empty())
                {
                    results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyVertices>());
                    continue;
                }

                if (indices.empty())
                {
                    results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyIndices>());
                    continue;
                }

                const std::string meshName = std::format("{}_{}_{}", modelName, mesh.mName.C_Str(), meshIdx);
                results.emplace_back(SaveStaticMeshAsset(resPathStr, meshName, vertices, indices));
            }
        }
        importer.FreeScene();

        const ResourceInfo resInfo{ .Type = EAssetType::StaticMesh };
        json resMetadata{};
        resMetadata << resInfo << desc;
        const fs::path resMetaPath = MakeResourceMetadataPath(resPath);
        if (!SaveJsonToFile(resMetaPath, resMetadata))
        {
            IG_LOG(ModelImporter, Warning, "Failed to save resource metadata to {}.", resMetaPath.string());
        }

        IG_CHECK(results.size() > 0);
        return results;
    }

    std::optional<StaticMesh> StaticMeshLoader::Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager)
    {
        IG_LOG(ModelImporter, Info, "Load static mesh {}.", guid.str());
        TempTimer tempTimer;
        tempTimer.Begin();

        const fs::path assetPath = MakeAssetPath(EAssetType::StaticMesh, guid);
        const fs::path assetMetaPath = MakeAssetMetadataPath(EAssetType::StaticMesh, guid);
        if (!fs::exists(assetPath))
        {
            IG_LOG(ModelImporter, Error, "Static Mesh Asset \"{}\" does not exists.", assetPath.string());
            return std::nullopt;
        }

        if (!fs::exists(assetMetaPath))
        {
            IG_LOG(ModelImporter, Error, "Static Mesh Asset metadata \"{}\" does not exists.", assetMetaPath.string());
            return std::nullopt;
        }

        const json assetMetadata{ LoadJsonFromFile(assetMetaPath) };
        if (assetMetadata.empty())
        {
            IG_LOG(ModelImporter, Error, "Failed to load asset metadata from {}.", assetMetaPath.string());
            return std::nullopt;
        }

        AssetInfo assetInfo{};
        StaticMeshLoadDesc loadDesc{};
        assetMetadata >> loadDesc >> assetInfo;

        if (assetInfo.Guid != guid)
        {
            IG_LOG(ModelImporter, Error, "Asset guid does not match. Expected: {}, Found: {}",
                   guid.str(), assetInfo.Guid.str());
            return std::nullopt;
        }

        if (assetInfo.Type != EAssetType::StaticMesh)
        {
            IG_LOG(ModelImporter, Error, "Asset type does not match. Expected: {}, Found: {}",
                   magic_enum::enum_name(EAssetType::StaticMesh),
                   magic_enum::enum_name(assetInfo.Type));
            return std::nullopt;
        }

        if (loadDesc.NumVertices == 0 ||
            loadDesc.NumIndices == 0 ||
            loadDesc.CompressedVerticesSizeInBytes == 0 ||
            loadDesc.CompressedIndicesSizeInBytes == 0)
        {
            IG_LOG(ModelImporter, Error,
                   "Load config has invalid values. \n * NumVertices: {}\n * NumIndices: {}\n * CompressedVerticesSizeInBytes: {}\n * CompressedIndicesSizeInBytes: {}",
                   loadDesc.NumVertices, loadDesc.NumIndices, loadDesc.CompressedVerticesSizeInBytes, loadDesc.CompressedIndicesSizeInBytes);
            return std::nullopt;
        }

        std::vector<uint8_t> blob = LoadBlobFromFile(assetPath);
        if (blob.empty())
        {
            IG_LOG(ModelImporter, Error, "Static Mesh {} blob is empty.", assetPath.string());
            return std::nullopt;
        }

        const size_t expectedBlobSize = loadDesc.CompressedVerticesSizeInBytes + loadDesc.CompressedIndicesSizeInBytes;
        if (blob.size() != expectedBlobSize)
        {
            IG_LOG(ModelImporter, Error, "Static Mesh blob size does not match. Expected: {}, Found: {}", expectedBlobSize, blob.size());
            return std::nullopt;
        }

        GpuBufferDesc vertexBufferDesc{};
        vertexBufferDesc.AsStructuredBuffer<StaticMeshVertex>(loadDesc.NumVertices);
        vertexBufferDesc.DebugName = String(std::format("{}_Vertices", guid.str()));
        if (vertexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedVerticesSizeInBytes)
        {
            IG_LOG(ModelImporter, Error, "Compressed vertices size exceed expected vertex buffer size. Compressed Size: {} bytes, Expected Vertex Buffer Size: {} bytes",
                   loadDesc.CompressedVerticesSizeInBytes,
                   vertexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        GpuBufferDesc indexBufferDesc{};
        indexBufferDesc.AsIndexBuffer<uint32_t>(loadDesc.NumIndices);
        indexBufferDesc.DebugName = String(std::format("{}_Indices", guid.str()));
        if (indexBufferDesc.GetSizeAsBytes() < loadDesc.CompressedIndicesSizeInBytes)
        {
            IG_LOG(ModelImporter, Error, "Compressed indices size exceed expected index buffer size. Compressed Size: {} bytes, Expected Index Buffer Size: {} bytes",
                   loadDesc.CompressedIndicesSizeInBytes,
                   indexBufferDesc.GetSizeAsBytes());
            return std::nullopt;
        }

        std::optional<GpuBuffer> vertexBuffer = renderDevice.CreateBuffer(vertexBufferDesc);
        if (!vertexBuffer)
        {
            IG_LOG(ModelImporter, Error, "Failed to create vertex buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        auto vertexBufferSrv = gpuViewManager.RequestShaderResourceView(*vertexBuffer);
        if (!vertexBufferSrv)
        {
            IG_LOG(ModelImporter, Error, "Failed to create vertex buffer shader resource view of {}.", assetPath.string());
            return std::nullopt;
        }

        std::optional<GpuBuffer> indexBuffer = renderDevice.CreateBuffer(indexBufferDesc);
        if (!indexBuffer)
        {
            IG_LOG(ModelImporter, Error, "Failed to create index buffer of {}.", assetPath.string());
            return std::nullopt;
        }

        UploadContext verticesUploadCtx = gpuUploader.Reserve(vertexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedVerticesOffset = 0;
            const int decodeResult = meshopt_decodeVertexBuffer(verticesUploadCtx.GetOffsettedCpuAddress(), loadDesc.NumVertices,
                                                                sizeof(StaticMeshVertex), blob.data() + compressedVerticesOffset, loadDesc.CompressedVerticesSizeInBytes);
            if (decodeResult == 0)
            {
                verticesUploadCtx.CopyBuffer(0, vertexBufferDesc.GetSizeAsBytes(), *vertexBuffer);
            }
            else
            {
                IG_LOG(ModelImporter, Error, "Failed to decode vertex buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> verticesUploadSync = gpuUploader.Submit(verticesUploadCtx);
        IG_CHECK(verticesUploadSync);

        UploadContext indicesUploadCtx = gpuUploader.Reserve(indexBufferDesc.GetSizeAsBytes());
        {
            const size_t compressedIndicesOffset = loadDesc.CompressedVerticesSizeInBytes;
            const int decodeResult = meshopt_decodeIndexBuffer<uint32_t>(
                reinterpret_cast<uint32_t*>(indicesUploadCtx.GetOffsettedCpuAddress()),
                loadDesc.NumIndices,
                blob.data() + compressedIndicesOffset, loadDesc.CompressedIndicesSizeInBytes);

            if (decodeResult == 0)
            {
                indicesUploadCtx.CopyBuffer(0, indexBufferDesc.GetSizeAsBytes(), *indexBuffer);
            }
            else
            {
                IG_LOG(ModelImporter, Error, "Failed to decode index buffer of {}.", assetPath.string());
            }
        }
        std::optional<GpuSync> indicesUploadSync = gpuUploader.Submit(indicesUploadCtx);
        IG_CHECK(indicesUploadSync);

        verticesUploadSync->WaitOnCpu();
        indicesUploadSync->WaitOnCpu();

        IG_LOG(ModelImporter, Info,
               "Successfully load static mesh asset {}, which from resource {}. Elapsed: {} ms",
               assetPath.string(), assetInfo.VirtualPath.ToStringView(), tempTimer.End());

        return StaticMesh{
            { assetInfo, loadDesc },
            { handleManager, std::move(*vertexBuffer) },
            std::move(vertexBufferSrv),
            { handleManager, std::move(*indexBuffer) }
        };
    }

    StaticMesh::StaticMesh(Desc snapshot, DeferredHandle<GpuBuffer> vertexBuffer, Handle<GpuView, GpuViewManager*> vertexBufferSrv, DeferredHandle<GpuBuffer> indexBuffer)
        : snapshot(snapshot),
          vertexBuffer(std::move(vertexBuffer)),
          vertexBufferSrv(std::move(vertexBufferSrv)),
          indexBuffer(std::move(indexBuffer))
    {
    }

    StaticMesh::~StaticMesh()
    {
    }
} // namespace ig