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
    {}

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
            Vector<MeshData> staticMeshes{scene->mNumMeshes};

            tf::Executor& taskExecutor = Engine::GetTaskExecutor();
            tf::Taskflow meshImportFlow;
            tf::Task procMeshes = meshImportFlow.for_each_index(
                0, (S32)scene->mNumMeshes, 1,
                [scene, &results, &staticMeshes, &desc, &modelName](const Index meshIdx)
                {
                    const aiMesh& mesh = *scene->mMeshes[meshIdx];
                    ProcessMeshLod0(mesh, staticMeshes[meshIdx]);

                    if (staticMeshes[meshIdx].Vertices.empty())
                    {
                        results[meshIdx] = MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyVertices>();
                        return;
                    }
                    if (staticMeshes[meshIdx].LevelOfDetails[0].Indices.empty())
                    {
                        results[meshIdx] = MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::EmptyIndices>();
                        return;
                    }

                    if (desc.bGenerateLODs)
                    {
                        GenerateLevelOfDetails(staticMeshes[meshIdx]);
                    }
                    IG_CHECK(staticMeshes[meshIdx].NumLevelOfDetails >= 1 && staticMeshes[meshIdx].NumLevelOfDetails <= StaticMesh::kMaxNumLods);

                    BuildMeshlets(staticMeshes[meshIdx]);
                    CompressMeshVertices(staticMeshes[meshIdx]);

                    const std::string meshName = std::format("{}_{}_{}", modelName, mesh.mName.C_Str(), meshIdx);
                    results[meshIdx] = ExportToFile(meshName, staticMeshes[meshIdx]);
                });
            taskExecutor.run(meshImportFlow).wait();
        }
        importer.FreeScene();

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
        importFlags |= aiProcess_GenSmoothNormals;
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

    void StaticMeshImporter::ProcessMeshLod0(const aiMesh& mesh, MeshData& meshData)
    {
        meshData.Vertices.reserve(mesh.mNumVertices);
        meshData.LevelOfDetails[0].Indices.reserve((Size)mesh.mNumFaces * Mesh::kNumVertexPerTriangle);

        for (Size vertexIdx = 0; vertexIdx < mesh.mNumVertices; ++vertexIdx)
        {
            const aiVector3D position = mesh.mVertices[vertexIdx];
            const aiVector3D normal = mesh.HasNormals() ? mesh.mNormals[vertexIdx] : aiVector3D(0.f, 0.f, 0.f);
            const aiVector3D uvCoords = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][vertexIdx] : aiVector3D(0.f, 0.f, 0.f);
            const aiVector3D tangent = mesh.HasTangentsAndBitangents() ? mesh.mTangents[vertexIdx] : aiVector3D(0.f, 0.f, 0.f);
            const aiVector3D bitangent = mesh.HasTangentsAndBitangents() ? mesh.mBitangents[vertexIdx] : aiVector3D(0.f, 0.f, 0.f);
            const aiColor4D vertexColor = mesh.HasVertexColors(0) ? mesh.mColors[0][vertexIdx] : aiColor4D(0.f, 0.f, 0.f, 1.f);

            meshData.BoundingBox.Min.x = std::min(meshData.BoundingBox.Min.x, position.x);
            meshData.BoundingBox.Min.y = std::min(meshData.BoundingBox.Min.y, position.y);
            meshData.BoundingBox.Min.z = std::min(meshData.BoundingBox.Min.z, position.z);
            meshData.BoundingBox.Max.x = std::max(meshData.BoundingBox.Max.x, position.x);
            meshData.BoundingBox.Max.y = std::max(meshData.BoundingBox.Max.y, position.y);
            meshData.BoundingBox.Max.z = std::max(meshData.BoundingBox.Max.z, position.z);

            Vertex newVertex{};
            newVertex.Position = Vector3{position.x, position.y, position.z};
            newVertex.QuantizedNormal = EncodeNormalX10Y10Z10(Vector3{normal.x, normal.y, normal.z});
            newVertex.QuantizedTangent = EncodeNormalX10Y10Z10(Vector3{tangent.x, tangent.y, tangent.z});
            newVertex.QuantizedBitangent = EncodeNormalX10Y10Z10(Vector3{bitangent.x, bitangent.y, bitangent.z});
            newVertex.QuantizedTexCoords[0] = meshopt_quantizeHalf(uvCoords.x);
            newVertex.QuantizedTexCoords[1] = meshopt_quantizeHalf(uvCoords.y);
            newVertex.ColorRGBA8_U32 = EncodeRGBA32F(Vector4{vertexColor.r, vertexColor.g, vertexColor.b, vertexColor.a});
            meshData.Vertices.emplace_back(newVertex);
        }

        for (Size faceIdx = 0; faceIdx < mesh.mNumFaces; ++faceIdx)
        {
            const aiFace& face = mesh.mFaces[faceIdx];
            IG_CHECK(face.mNumIndices == NumIndicesPerFace);
            meshData.LevelOfDetails[0].Indices.emplace_back(face.mIndices[0]);
            meshData.LevelOfDetails[0].Indices.emplace_back(face.mIndices[1]);
            meshData.LevelOfDetails[0].Indices.emplace_back(face.mIndices[2]);
        }

        IG_CHECK(meshData.Vertices.size() == mesh.mNumVertices);
        IG_CHECK(meshData.LevelOfDetails[0].Indices.size() == ((Size)mesh.mNumFaces * Mesh::kNumVertexPerTriangle));
        IG_CHECK(meshData.NumLevelOfDetails == 1);
    }

    void StaticMeshImporter::GenerateLevelOfDetails(MeshData& meshData)
    {
        IG_CHECK(meshData.NumLevelOfDetails == 1);

        const Vector<Vertex>& verticesLod0 = meshData.Vertices;
        const Vector<U32>& indicesLod0 = meshData.LevelOfDetails[0].Indices;
        for (Index lod = 1; lod < Mesh::kMaxMeshLevelOfDetails; ++lod)
        {
            const Size previousLodNumIndices = meshData.LevelOfDetails[lod - 1].Indices.size();
            const Size targetNumIndices = (Size)((F32)previousLodNumIndices * 0.7f);
            if (targetNumIndices == 0)
            {
                break;
            }

            Vector<U32>& lodIndices = meshData.LevelOfDetails[lod].Indices;
            lodIndices.resize(indicesLod0.size());
            const F32 tLod = (F32)lod / (F32)Mesh::kMaxMeshLevelOfDetails;
            const float targetError = (1.f - tLod) * 0.05f + tLod * 0.95f;

            Size numSimplifiedIndices = meshopt_simplify(
                lodIndices.data(),
                indicesLod0.data(), indicesLod0.size(),
                &verticesLod0[0].Position.x, verticesLod0.size(), sizeof(Vertex),
                targetNumIndices, targetError,
                0, nullptr);

            bool simplifyPass = numSimplifiedIndices <= targetNumIndices;
            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplifySloppy(
                    lodIndices.data(),
                    indicesLod0.data(), indicesLod0.size(),
                    &verticesLod0[0].Position.x, verticesLod0.size(), sizeof(Vertex),
                    targetNumIndices, targetError, nullptr);

                simplifyPass = numSimplifiedIndices <= targetNumIndices;
            }

            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplify(
                    lodIndices.data(),
                    indicesLod0.data(), indicesLod0.size(),
                    &verticesLod0[0].Position.x, verticesLod0.size(), sizeof(Vertex),
                    targetNumIndices, FLT_MAX,
                    0, nullptr);

                simplifyPass = numSimplifiedIndices <= targetNumIndices;
            }

            if (!simplifyPass)
            {
                numSimplifiedIndices = meshopt_simplifySloppy(
                    lodIndices.data(),
                    indicesLod0.data(), indicesLod0.size(),
                    &verticesLod0[0].Position.x, verticesLod0.size(), sizeof(Vertex),
                    targetNumIndices, FLT_MAX, nullptr);

                simplifyPass = numSimplifiedIndices < previousLodNumIndices;
            }

            if (!simplifyPass)
            {
                lodIndices.clear();
                break;
            }
            lodIndices.resize(numSimplifiedIndices);
            if (numSimplifiedIndices == 0)
            {
                break;
            }

            ++meshData.NumLevelOfDetails;
        }
    }

    void StaticMeshImporter::BuildMeshlets(MeshData& meshData)
    {
        IG_CHECK(meshData.NumLevelOfDetails >= 1);
        IG_CHECK(!meshData.Vertices.empty());

        constexpr F32 kConeWeight = 0.f;
        Vector<meshopt_Meshlet> meshlets;
        Vector<U8> triangles;
        for (U8 lod = 0; lod < meshData.NumLevelOfDetails; ++lod)
        {
            MeshLod& meshLod = meshData.LevelOfDetails[lod];
            const Size maxMeshlets = meshopt_buildMeshletsBound(
                meshLod.Indices.size(),
                Meshlet::kMaxVertices,
                Meshlet::kMaxTriangles);
            meshlets.resize(maxMeshlets);
            meshLod.MeshletVertexIndices.resize(maxMeshlets * Meshlet::kMaxVertices);
            triangles.resize(maxMeshlets * Meshlet::kMaxTriangles * Mesh::kNumVertexPerTriangle);

            const Size numMeshlets = meshopt_buildMeshlets(
                meshlets.data(),
                meshLod.MeshletVertexIndices.data(),
                triangles.data(),
                meshLod.Indices.data(), meshLod.Indices.size(),
                &meshData.Vertices[0].Position.x, meshData.Vertices.size(), sizeof(Vertex),
                Meshlet::kMaxVertices, Meshlet::kMaxTriangles, kConeWeight);

            Size numIndices = 0;
            U32 numTriangles = 0;
            meshLod.Meshlets.resize(numMeshlets);
            meshLod.MeshletTriangles.reserve(numMeshlets * Meshlet::kMaxTriangles);
            for (Index meshletIdx = 0; meshletIdx < numMeshlets; ++meshletIdx)
            {
                const meshopt_Meshlet& meshOptMeshlet = meshlets[meshletIdx];
                meshopt_optimizeMeshlet(
                    meshLod.MeshletVertexIndices.data() + meshOptMeshlet.vertex_offset,
                    triangles.data() + meshOptMeshlet.triangle_offset,
                    meshOptMeshlet.triangle_count,
                    meshOptMeshlet.vertex_count);

                const meshopt_Bounds meshletBounds = meshopt_computeMeshletBounds(
                    meshLod.MeshletVertexIndices.data() + meshOptMeshlet.vertex_offset,
                    triangles.data() + meshOptMeshlet.triangle_offset, meshOptMeshlet.triangle_count,
                    &meshData.Vertices[0].Position.x, meshData.Vertices.size(), sizeof(Vertex));

                Meshlet& meshlet = meshLod.Meshlets[meshletIdx];
                meshlet.IndexOffset = meshOptMeshlet.vertex_offset;
                meshlet.NumIndices = meshOptMeshlet.vertex_count;
                numIndices += meshOptMeshlet.vertex_count;

                meshlet.TriangleOffset = numTriangles;
                meshlet.NumTriangles = meshOptMeshlet.triangle_count;
                for (Index triangleIdx = 0; triangleIdx < meshOptMeshlet.triangle_count; ++triangleIdx)
                {
                    meshLod.MeshletTriangles.emplace_back(
                        EncodeTriangleU32(
                            triangles[meshOptMeshlet.triangle_offset + triangleIdx * Mesh::kNumVertexPerTriangle + 0],
                            triangles[meshOptMeshlet.triangle_offset + triangleIdx * Mesh::kNumVertexPerTriangle + 1],
                            triangles[meshOptMeshlet.triangle_offset + triangleIdx * Mesh::kNumVertexPerTriangle + 2]));
                }
                numTriangles += meshOptMeshlet.triangle_count;

                meshlet.BoundingVolume = BoundingSphere{
                    Vector3{meshletBounds.center[0], meshletBounds.center[1], meshletBounds.center[2]},
                    meshletBounds.radius
                };

                meshlet.QuantizedNormalConeAxis[0] = (U8)(meshletBounds.cone_axis_s8[0] + 127);
                meshlet.QuantizedNormalConeAxis[1] = (U8)(meshletBounds.cone_axis_s8[1] + 127);
                meshlet.QuantizedNormalConeAxis[2] = (U8)(meshletBounds.cone_axis_s8[2] + 127);
                meshlet.QuantizedNormalConeCutoff = (U8)(meshletBounds.cone_cutoff_s8 + 127);
            }

            meshLod.MeshletVertexIndices.resize(numIndices);
        }
    }

    void StaticMeshImporter::CompressMeshVertices(MeshData& meshData)
    {
        IG_CHECK(meshData.NumLevelOfDetails >= 1);
        IG_CHECK(!meshData.Vertices.empty());

        meshopt_encodeVertexVersion(0);
        meshopt_encodeIndexVersion(1);

        meshData.CompressedVertices.resize(
            meshopt_encodeVertexBufferBound(meshData.Vertices.size(), sizeof(Vertex)));
        meshData.CompressedVertices.resize(
            meshopt_encodeVertexBuffer(meshData.CompressedVertices.data(),
                meshData.CompressedVertices.size(),
                meshData.Vertices.data(),
                meshData.Vertices.size(),
                sizeof(Vertex)));
    }

    Result<StaticMesh::Desc, EStaticMeshImportStatus> StaticMeshImporter::ExportToFile(const std::string_view meshName, const MeshData& meshData)
    {
        const AssetInfo assetInfo{MakeVirtualPathPreferred(meshName), EAssetCategory::StaticMesh};

        StaticMeshLoadDesc newLoadDesc{};
        newLoadDesc.NumVertices = (U32)meshData.Vertices.size();
        newLoadDesc.CompressedVerticesSize = (U32)meshData.CompressedVertices.size();
        newLoadDesc.NumLevelOfDetails = meshData.NumLevelOfDetails;
        for (U8 lod = 0; lod < meshData.NumLevelOfDetails; ++lod)
        {
            const MeshLod& meshLod = meshData.LevelOfDetails[lod];
            newLoadDesc.NumMeshletVertexIndices[lod] = (U32)meshLod.MeshletVertexIndices.size();
            newLoadDesc.NumMeshletTriangles[lod] = (U32)meshLod.MeshletTriangles.size();
            newLoadDesc.NumMeshlets[lod] = (U32)meshLod.Meshlets.size();
        }
        newLoadDesc.BoundingBox = meshData.BoundingBox;

        const Path newMetaPath = MakeAssetMetadataPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());

        Json assetMetadata{};
        assetMetadata << assetInfo << newLoadDesc;
        if (!SaveJsonToFile(newMetaPath, assetMetadata))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveMetadataToFile>();
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::StaticMesh, assetInfo.GetGuid());
        constexpr Size kNumBlobPerLod = 3;     /* MeshletVertexIndices, MeshletTriangles, Meshlets */
        constexpr Size kNumAdditionalBlob = 1; /* CompressedVertices */
        constexpr Size kNumBlobs = (kNumBlobPerLod * Mesh::kMaxMeshLevelOfDetails) + kNumAdditionalBlob;
        Array<std::span<const U8>, kNumBlobs> blobs{};
        blobs[0] = std::span<const U8>{meshData.CompressedVertices};
        for (U8 lod = 0; lod < meshData.NumLevelOfDetails; ++lod)
        {
            const MeshLod& meshLod = meshData.LevelOfDetails[lod];
            const Index blobOffset = kNumAdditionalBlob + (lod * kNumBlobPerLod);
            blobs[blobOffset + 0] = std::span<const U8>{
                reinterpret_cast<const U8*>(meshLod.MeshletVertexIndices.data()),
                sizeof(U32) * meshLod.MeshletVertexIndices.size()
            };
            blobs[blobOffset + 1] = std::span<const U8>{
                reinterpret_cast<const U8*>(meshLod.MeshletTriangles.data()),
                sizeof(U32) * meshLod.MeshletTriangles.size()
            };
            blobs[blobOffset + 2] = std::span<const U8>{
                reinterpret_cast<const U8*>(meshLod.Meshlets.data()),
                sizeof(Meshlet) * meshLod.Meshlets.size()
            };
        }
        if (!SaveBlobsToFile(assetPath, blobs))
        {
            return MakeFail<StaticMesh::Desc, EStaticMeshImportStatus::FailedSaveAssetToFile>();
        }

        IG_CHECK(assetInfo.IsValid());
        return MakeSuccess<StaticMesh::Desc, EStaticMeshImportStatus>(assetInfo, newLoadDesc);
    }
} // namespace ig
