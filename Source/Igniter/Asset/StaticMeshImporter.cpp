#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Render/Vertex.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMeshImporter.h"

IG_DECLARE_LOG_CATEGORY(StaticMeshImporterLog);
IG_DEFINE_LOG_CATEGORY(StaticMeshImporterLog);

namespace ig
{
    constexpr inline Size NumIndicesPerFace = 3;

    StaticMeshImporter::StaticMeshImporter(AssetManager& assetManager)
        : assetManager(assetManager)
    {
    }

    static bool CheckAssimpSceneLoadingSucceed(const String resPathStr, const Assimp::Importer& importer, const aiScene* scene)
    {
        if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
        {
            IG_LOG(StaticMeshImporterLog, Error, "Load model file from \"{}\" failed: \"{}\"", resPathStr.ToStringView(), importer.GetErrorString());
            return false;
        }

        return true;
    }

    Vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> StaticMeshImporter::Import(const String resPathStr, const StaticMesh::ImportDesc& desc)
    {
        Vector<Result<StaticMesh::Desc, EStaticMeshImportStatus>> results;
        const Path resPath{resPathStr.ToStringView()};
        if (!fs::exists(resPath))
        {
            results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FileDoesNotExists>());
            return results;
        }

        const U32 importFlags = MakeAssimpImportFlagsFromDesc(desc);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(resPathStr.ToStandard(), importFlags);
        {
            if (!CheckAssimpSceneLoadingSucceed(resPathStr, importer, scene))
            {
                results.emplace_back(MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedLoadFromFile>());
                return results;
            }

            /* Create Materials */
            if (desc.bImportMaterials)
            {
                const Size numImportedMaterials = ImportMaterialsFromScene(assetManager, *scene);
                IG_LOG(StaticMeshImporterLog, Info, "{} of materials imported from static mesh asset.", numImportedMaterials);
            }

            /* Import Static Meshes */
            const std::string modelName = resPath.filename().replace_extension().string();
            results.resize(scene->mNumMeshes);
            Vector<StaticMeshData> staticMeshes{scene->mNumMeshes};
            Vector<CompressedStaticMeshData> compressedStaticMeshes{scene->mNumMeshes};

            tf::Executor& taskExecutor = Engine::GetTaskExecutor();
            tf::Taskflow meshImportFlow;

            tf::Task processStaticMeshData = meshImportFlow.for_each_index(
                0, (I32)scene->mNumMeshes, 1,
                [scene, &results, &staticMeshes, &compressedStaticMeshes, &desc, &modelName](const Index meshIdx)
                {
                    const aiMesh& mesh = *scene->mMeshes[meshIdx];
                    staticMeshes[meshIdx] = ProcessMeshData(mesh);

                    if (staticMeshes[meshIdx].Vertices.empty())
                    {
                        results[meshIdx] = MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyVertices>();
                        return;
                    }
                    if (staticMeshes[meshIdx].IndicesPerLod[0].empty())
                    {
                        results[meshIdx] = MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyIndices>();
                        return;
                    }

                    OptimizeMeshData(staticMeshes[meshIdx], desc);

                    if (desc.bGenerateLODs)
                    {
                        GenerateLODs(staticMeshes[meshIdx],
                                     std::clamp(desc.NumLods, (U8)2, StaticMesh::kMaxNumLods),
                                     desc.MaxSimplificationFactor);
                    }
                    IG_CHECK(staticMeshes[meshIdx].NumLods >= 1 && staticMeshes[meshIdx].NumLods <= StaticMesh::kMaxNumLods);

                    compressedStaticMeshes[meshIdx] = CompressMeshData(staticMeshes[meshIdx]);

                    const std::string meshName = std::format("{}_{}_{}", modelName, mesh.mName.C_Str(), meshIdx);
                    results[meshIdx] = SaveAsAsset(meshName, staticMeshes[meshIdx], compressedStaticMeshes[meshIdx]);
                });

            taskExecutor.run(meshImportFlow).wait();
        }
        importer.FreeScene();

        const ResourceInfo resInfo{.Category = EAssetCategory::StaticMesh};
        Json resMetadata{};
        resMetadata << resInfo << desc;
        const Path resMetaPath = MakeResourceMetadataPath(resPath);
        if (!SaveJsonToFile(resMetaPath, resMetadata))
        {
            IG_LOG(StaticMeshImporterLog, Warning, "Failed to save resource metadata to {}.", resMetaPath.string());
        }

        IG_CHECK(results.size() > 0);
        return results;
    }

    U32 StaticMeshImporter::MakeAssimpImportFlagsFromDesc(const StaticMesh::ImportDesc& desc)
    {
        U32 importFlags = aiProcess_Triangulate;
        importFlags |= desc.bMakeLeftHanded ? aiProcess_MakeLeftHanded : 0;
        importFlags |= desc.bFlipUVs ? aiProcess_FlipUVs : 0;
        importFlags |= desc.bFlipWindingOrder ? aiProcess_FlipWindingOrder : 0;
        importFlags |= desc.bSplitLargeMeshes ? aiProcess_SplitLargeMeshes : 0;
        importFlags |= desc.bPreTransformVertices ? aiProcess_PreTransformVertices : 0;
        importFlags |= desc.bGenerateNormals ? aiProcess_GenSmoothNormals : 0;
        importFlags |= desc.bGenerateUVCoords ? aiProcess_GenUVCoords : 0;
        importFlags |= desc.bGenerateBoundingBoxes ? aiProcess_GenBoundingBoxes : 0;
        return importFlags;
    }

    Size StaticMeshImporter::ImportMaterialsFromScene(AssetManager& assetManager, const aiScene& scene)
    {
        Size numImportedMaterials = 0;
        for (U32 materialIdx = 0; materialIdx < scene.mNumMaterials; ++materialIdx)
        {
            const aiMaterial& material = *scene.mMaterials[materialIdx];
            const Guid importedGuid = assetManager.Import(MakeVirtualPathPreferred(String(material.GetName().C_Str())),
                                                          MaterialAssetCreateDesc{.DiffuseVirtualPath = Texture::EngineDefault});
            numImportedMaterials = importedGuid.isValid() ? (numImportedMaterials + 1) : numImportedMaterials;
        }
        return numImportedMaterials;
    }

    StaticMeshImporter::StaticMeshData StaticMeshImporter::ProcessMeshData(const aiMesh& mesh)
    {
        StaticMeshData newData{};
        newData.Vertices.reserve(mesh.mNumVertices);
        newData.IndicesPerLod[0].reserve(mesh.mNumFaces * NumIndicesPerFace);

        for (Size vertexIdx = 0; vertexIdx < mesh.mNumVertices; ++vertexIdx)
        {
            const aiVector3D& position = mesh.mVertices[vertexIdx];
            const aiVector3D& normal = mesh.mNormals[vertexIdx];
            const aiVector3D& uvCoords = mesh.mTextureCoords[0][vertexIdx];

            newData.BoundingVolume.Min.x = std::min(newData.BoundingVolume.Min.x, position.x);
            newData.BoundingVolume.Min.y = std::min(newData.BoundingVolume.Min.y, position.y);
            newData.BoundingVolume.Min.z = std::min(newData.BoundingVolume.Min.z, position.z);
            newData.BoundingVolume.Max.x = std::max(newData.BoundingVolume.Max.x, position.x);
            newData.BoundingVolume.Max.y = std::max(newData.BoundingVolume.Max.y, position.y);
            newData.BoundingVolume.Max.z = std::max(newData.BoundingVolume.Max.z, position.z);

            newData.Vertices.emplace_back(
                Vector3{position.x, position.y, position.z},
                Vector3{normal.x, normal.y, normal.z},
                Vector2{uvCoords.x, uvCoords.y});
        }

        for (Size faceIdx = 0; faceIdx < mesh.mNumFaces; ++faceIdx)
        {
            const aiFace& face = mesh.mFaces[faceIdx];
            IG_CHECK(face.mNumIndices == NumIndicesPerFace);
            newData.IndicesPerLod[0].emplace_back(face.mIndices[0]);
            newData.IndicesPerLod[0].emplace_back(face.mIndices[1]);
            newData.IndicesPerLod[0].emplace_back(face.mIndices[2]);
        }

        return newData;
    }

    void StaticMeshImporter::OptimizeMeshData(StaticMeshData& meshData, const StaticMeshImportDesc& desc)
    {
        Vector<VertexSM>& vertices = meshData.Vertices;
        Vector<U32>& indices = meshData.IndicesPerLod[0];

        Vector<U32> remap(indices.size());
        const Size remappedVertexCount =
            meshopt_generateVertexRemap(
                remap.data(),
                indices.data(), indices.size(),
                vertices.data(), indices.size(),
                sizeof(VertexSM));

        Vector<U32> remappedIndices(indices.size());
        Vector<VertexSM> remappedVertices(remappedVertexCount);

        meshopt_remapIndexBuffer(
            remappedIndices.data(),
            indices.data(), indices.size(),
            remap.data());

        meshopt_remapVertexBuffer(
            remappedVertices.data(),
            vertices.data(), vertices.size(),
            sizeof(VertexSM),
            remap.data());

        meshopt_optimizeVertexCache(
            remappedIndices.data(), remappedIndices.data(), remappedIndices.size(),
            remappedVertices.size());

        meshopt_optimizeOverdraw(
            remappedIndices.data(), remappedIndices.data(), remappedIndices.size(),
            &remappedVertices[0].Position.x, remappedVertices.size(), sizeof(VertexSM),
            desc.OverdrawOptimizerThreshold);

        meshopt_optimizeVertexFetch(
            remappedVertices.data(), remappedIndices.data(), remappedIndices.size(),
            remappedVertices.data(), remappedVertices.size(), sizeof(VertexSM));
    }

    void ig::StaticMeshImporter::GenerateLODs(StaticMeshData& meshData, const U8 numLods, const F32 maxSimplificationFactor)
    {
        IG_CHECK(numLods >= 2 && numLods <= StaticMesh::kMaxNumLods);
        meshData.NumLods = 1;

        const Vector<VertexSM>& rootLodVertices = meshData.Vertices;
        const Vector<U32>& rootLodIndices = meshData.IndicesPerLod[0];
        // LOD 0 (최소 1개 LOD)는 LOD 생성에서 제외(원본)
        const F32 simplificationFactorStride = maxSimplificationFactor / (F32)numLods;
        for (Index lod = 1; lod < numLods; ++lod)
        {
            const Size previousLodNumIndices = meshData.IndicesPerLod[lod - 1].size();
            const Size targetNumIndices = std::max(3llu, (Size)((F32)rootLodIndices.size() * (1.f - (simplificationFactorStride * (F32)lod))));
            Vector<U32>& lodIndices = meshData.IndicesPerLod[lod];
            lodIndices.resize(rootLodIndices.size());

            // 1. Simplify 함수로 error value 0.02(2% deviation); 이전 대비 최소 5% 이상 감소시 통과
            Size numSimplifiedIndices = meshopt_simplify(
                lodIndices.data(),
                rootLodIndices.data(), rootLodIndices.size(),
                &rootLodVertices[0].Position.x, rootLodVertices.size(), sizeof(VertexSM),
                targetNumIndices, 0.02f,
                0, nullptr);

            const Size simplifyOptThreshold = (Size)(previousLodNumIndices * 0.95f);
            const Size simplifySloppyOptThreshold = (Size)(previousLodNumIndices * 0.9f);
            bool simplifyPass = numSimplifiedIndices <= simplifyOptThreshold;

            // 2. Simplify 함수로 error value 무제한; 이전 대비 최소 5% 이상 감소시 통과
            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplify(
                    lodIndices.data(),
                    rootLodIndices.data(), rootLodIndices.size(),
                    &rootLodVertices[0].Position.x, rootLodVertices.size(), sizeof(VertexSM),
                    targetNumIndices, FLT_MAX,
                    0, nullptr);

                simplifyPass = numSimplifiedIndices <= simplifyOptThreshold;
            }

            // 3. SimplifySloppy 함수로 error value 0.05(5% deviation); 이전 대비 최소 10% 이상 감소시 통과;
            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplifySloppy(
                    lodIndices.data(),
                    rootLodIndices.data(), rootLodIndices.size(),
                    &rootLodVertices[0].Position.x, rootLodVertices.size(), sizeof(VertexSM),
                    targetNumIndices, 0.1f, nullptr);

                simplifyPass = numSimplifiedIndices <= simplifySloppyOptThreshold;
            }

            // 4. SimplifySloppy 함수로 error value 무제한; 이전 대비 감소 없을 시, 이 이상의 lod 생성은 불가능 판단, 생성 종료
            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplifySloppy(
                    lodIndices.data(),
                    rootLodIndices.data(), rootLodIndices.size(),
                    &rootLodVertices[0].Position.x, rootLodVertices.size(), sizeof(VertexSM),
                    targetNumIndices, FLT_MAX, nullptr);

                simplifyPass = numSimplifiedIndices < previousLodNumIndices;
            }

            if (!simplifyPass)
            {
                lodIndices.clear();
                break;
            }

            ++meshData.NumLods;
            lodIndices.resize(numSimplifiedIndices);
            meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndices.size(), rootLodVertices.size());
        }
    }

    StaticMeshImporter::CompressedStaticMeshData StaticMeshImporter::CompressMeshData(const StaticMeshData& meshData)
    {
        CompressedStaticMeshData newCompressedData{};

        /* Vertex/Index buffer compression */
        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        newCompressedData.CompressedVertices.resize(
            meshopt_encodeVertexBufferBound(meshData.Vertices.size(), sizeof(VertexSM)));
        newCompressedData.CompressedVertices.resize(
            meshopt_encodeVertexBuffer(
                newCompressedData.CompressedVertices.data(), newCompressedData.CompressedVertices.size(),
                meshData.Vertices.data(), meshData.Vertices.size(), sizeof(VertexSM)));

        for (Index lod = 0; lod < meshData.NumLods; ++lod)
        {
            const Vector<U32>& indices{meshData.IndicesPerLod[lod]};
            Vector<U8>& compressedIndices{newCompressedData.CompressedIndicesPerLod[lod]};

            compressedIndices.resize(
                meshopt_encodeIndexBufferBound(indices.size(), meshData.Vertices.size()));
            compressedIndices.resize(meshopt_encodeIndexBuffer(
                compressedIndices.data(), compressedIndices.size(),
                indices.data(), indices.size()));
        }

        return newCompressedData;
    }

    Result<StaticMesh::Desc, EStaticMeshImportStatus> StaticMeshImporter::SaveAsAsset(const std::string_view meshName, const StaticMeshData& meshData, const CompressedStaticMeshData& compressedMeshData)
    {
        const AssetInfo assetInfo{MakeVirtualPathPreferred(meshName), EAssetCategory::StaticMesh};

        StaticMeshLoadDesc newLoadConfig{
            .NumVertices = (U32)meshData.Vertices.size(),
            .CompressedVerticesSizeInBytes = compressedMeshData.CompressedVertices.size(),
            .NumLods = meshData.NumLods,
            .AABB = meshData.BoundingVolume};

        for (Index lod = 0; lod < meshData.NumLods; ++lod)
        {
            newLoadConfig.NumIndicesPerLod[lod] =
                (U32)meshData.IndicesPerLod[lod].size();

            newLoadConfig.CompressedIndicesSizePerLod[lod] =
                (U32)compressedMeshData.CompressedIndicesPerLod[lod].size();
        }

        const Path newMetaPath = MakeAssetMetadataPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());

        Json assetMetadata{};
        assetMetadata << assetInfo << newLoadConfig;
        if (!SaveJsonToFile(newMetaPath, assetMetadata))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveMetadataToFile>();
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());
        Array<std::span<const U8>, StaticMesh::kMaxNumLods + 1> blobs{
            std::span<const U8>{compressedMeshData.CompressedVertices},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[0]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[1]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[2]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[3]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[4]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[5]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[6]},
            std::span<const U8>{compressedMeshData.CompressedIndicesPerLod[7]}};
        if (!SaveBlobsToFile(assetPath, blobs))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid());
        return MakeSuccess<StaticMesh::Desc, EStaticMeshImportStatus>(assetInfo, newLoadConfig);
    }
} // namespace ig
