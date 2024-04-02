#include <Igniter.h>
#include <Core/Log.h>
#include <Core/Timer.h>
#include <Filesystem/Utils.h>
#include <Render/Vertex.h>
#include <Asset/StaticMeshImporter.h>

IG_DEFINE_LOG_CATEGORY(StaticMeshImporter);

namespace ig
{
    constexpr inline size_t NumIndicesPerFace = 3;

    StaticMeshImporter::StaticMeshImporter(AssetManager& assetManager) : assetManager(assetManager)
    {
    }

    static bool CheckAssimpSceneLoadingSucceed(const String resPathStr, const Assimp::Importer& importer, const aiScene* scene)
    {
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            IG_LOG(StaticMeshImporter, Error, "Load model file from \"{}\" failed: \"{}\"", resPathStr.ToStringView(), importer.GetErrorString());
            return false;
        }

        return true;
    }

    static void ProcessStaticMeshData(const aiMesh& mesh, std::vector<StaticMeshVertex>& vertices,
                                      std::vector<uint32_t>& indices,
                                      const uint32_t vertexIdxOffset = 0)
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

    static Result<StaticMesh::Desc, EStaticMeshImportStatus> SaveStaticMeshAsset(const String resPathStr,
                                                                                 const std::string_view meshName,
                                                                                 const std::vector<StaticMeshVertex>& vertices,
                                                                                 const std::vector<uint32_t>& indices)
    {
        IG_CHECK(!vertices.empty());
        IG_CHECK(!indices.empty());
        IG_CHECK(resPathStr.IsValid());

        const std::string_view resPathStrView = resPathStr.ToStringView();

        std::vector<uint32_t> remap(indices.size());
        const size_t remappedVertexCount = meshopt_generateVertexRemap(remap.data(),
                                                                       indices.data(), indices.size(),
                                                                       vertices.data(), indices.size(), sizeof(StaticMeshVertex));

        std::vector<uint32_t> remappedIndices(indices.size());
        std::vector<StaticMeshVertex> remappedVertices(remappedVertexCount);

        meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
        meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(StaticMeshVertex), remap.data());
        IG_LOG(StaticMeshImporter, Info, "Remapped #Vertices {} -> {}.", vertices.size(), remappedVertexCount);

        constexpr float CacheHitRatioThreshold = 1.02f;
        meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(),
                                    remappedVertices.size());
        meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(),
                                 &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(StaticMeshVertex),
                                 CacheHitRatioThreshold);
        meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(),
                                    remappedVertices.data(), remappedVertices.size(), sizeof(StaticMeshVertex));

        /* #sy_ref https://www.realtimerendering.com/blog/acmr-and-atvr/ */
        IG_LOG(StaticMeshImporter, Info, "Optimization Statistics \"{}({})\"", resPathStrView, meshName);
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

        const auto os = meshopt_analyzeOverdraw(remappedIndices.data(), remappedIndices.size(),
                                                &remappedVertices[0].Position.x,
                                                remappedVertices.size(), sizeof(StaticMeshVertex));

        IG_LOG(StaticMeshImporter, Info, "Vertex Cache Statistics(NVIDIA) - ACMR: {} / ATVR: {}",
               nvidiaVcs.acmr, nvidiaVcs.atvr);
        IG_LOG(StaticMeshImporter, Info, "Vertex Cache Statistics(AMD) - ACMR: {} / ATVR: {}",
               amdVcs.acmr, amdVcs.atvr);
        IG_LOG(StaticMeshImporter, Info, "Vertex Cache Statistics(INTEL) - ACMR: {} / ATVR: {}",
               intelVcs.acmr, intelVcs.atvr);
        IG_LOG(StaticMeshImporter, Info, "Overfecth: {}", vfs.overfetch);
        IG_LOG(StaticMeshImporter, Info, "Overdraw: {}", os.overdraw);

        /* Vertex/Index buffer compression */
        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        std::vector<uint8_t> encodedVertices(meshopt_encodeVertexBufferBound(remappedVertices.size(),
                                                                             sizeof(StaticMeshVertex)));
        std::vector<uint8_t> encodedIndices(meshopt_encodeIndexBufferBound(remappedIndices.size(),
                                                                           remappedVertices.size()));

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
        if (!SaveBlobsToFile<2>(assetPath, { std::span<const uint8_t>{ encodedVertices },
                                             std::span<const uint8_t>{ encodedIndices } }))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid());
        IG_CHECK(newLoadConfig.NumVertices > 0 && newLoadConfig.NumIndices > 0);
        IG_CHECK(newLoadConfig.CompressedVerticesSizeInBytes > 0 && newLoadConfig.CompressedIndicesSizeInBytes);
        return MakeSuccess<StaticMesh::Desc, EStaticMeshImportStatus>(assetInfo, newLoadConfig);
    }

    std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> StaticMeshImporter::ImportStaticMesh(const String resPathStr,
                                                                                                        const StaticMesh::ImportDesc& desc)
    {
        std::vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results;
        const fs::path resPath{ resPathStr.ToStringView() };
        if (!fs::exists(resPath))
        {
            results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FileDoesNotExists>());
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
            IG_LOG(StaticMeshImporter, Warning, "Failed to save resource metadata to {}.", resMetaPath.string());
        }

        IG_CHECK(results.size() > 0);
        return results;
    }

} // namespace ig