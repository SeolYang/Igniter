#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMesh.h"

namespace ig
{
    Json& StaticMeshImportDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bMakeLeftHanded);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateNormals);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bSplitLargeMeshes);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bPreTransformVertices);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bImproveCacheLocality);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateUVCoords);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bFlipUVs);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bFlipWindingOrder);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateBoundingBoxes);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bImportMaterials);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateLODs);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, NumLods);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, MaxSimplificationFactor);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bOptimizeVertexCache);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bOptimizeOverdraw);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, OverdrawOptimizerThreshold);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bOptimizeVertexFetch);
        return archive;
    }

    const Json& StaticMeshImportDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bMakeLeftHanded);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bFlipUVs);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bFlipWindingOrder);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateNormals);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bSplitLargeMeshes);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bPreTransformVertices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bImproveCacheLocality);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateUVCoords);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateBoundingBoxes);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bImportMaterials);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateLODs);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, NumLods);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, MaxSimplificationFactor);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bOptimizeVertexCache);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bOptimizeOverdraw);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, OverdrawOptimizerThreshold);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bOptimizeVertexFetch);
        return archive;
    }

    Json& StaticMeshLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumVertices);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes);

        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumLods);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumIndicesPerLod);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, CompressedIndicesSizePerLod);

        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, AABB);

        return archive;
    }

    const Json& StaticMeshLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumVertices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes);

        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumLods);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumIndicesPerLod);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, CompressedIndicesSizePerLod);

        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, AABB);
        return archive;
    }

    StaticMesh::StaticMesh(
        RenderContext& renderContext, AssetManager& assetManager,
        const Desc& snapshot,
        const Handle<MeshStorage::Space<VertexSM>> vertexSpace,
        const U8 numLods,
        const Array<Handle<MeshStorage::Space<U32>>, kMaxNumLods> vertexIndexSpacePerLod)
        : renderContext(&renderContext)
        , assetManager(&assetManager)
        , snapshot(snapshot)
        , vertexSpace(vertexSpace)
        , numLods(numLods)
        , vertexIndexSpacePerLod(vertexIndexSpacePerLod)
    {
        IG_CHECK(vertexSpace.Value != 8);
    }

    StaticMesh::StaticMesh(StaticMesh&& other) noexcept
        : renderContext(std::exchange(other.renderContext, nullptr))
        , assetManager(std::exchange(other.assetManager, nullptr))
        , snapshot(other.snapshot)
        , vertexSpace(std::exchange(other.vertexSpace, {}))
        , numLods(std::exchange<U8, U8>(other.numLods, 0))
        , vertexIndexSpacePerLod(std::exchange(other.vertexIndexSpacePerLod, {}))
    {
        IG_CHECK(vertexSpace.Value != 8);
    }

    StaticMesh::~StaticMesh()
    {
        Destroy();
    }

    void StaticMesh::Destroy()
    {
        renderContext = nullptr;
        assetManager = nullptr;

        MeshStorage& meshStorage = Engine::GetMeshStorage();
        meshStorage.Destroy(vertexSpace);
        vertexSpace = {};

        for (Index lod = 0; lod < numLods; ++lod)
        {
            meshStorage.Destroy(vertexIndexSpacePerLod[lod]);
            vertexIndexSpacePerLod[lod] = {};
        }
        numLods = 0;
    }

    StaticMesh& StaticMesh::operator=(StaticMesh&& rhs) noexcept
    {
        Destroy();

        this->renderContext = std::exchange(rhs.renderContext, nullptr);
        this->assetManager = std::exchange(rhs.assetManager, nullptr);
        this->snapshot = rhs.snapshot;

        this->vertexSpace = std::exchange(rhs.vertexSpace, {});

        this->numLods = std::exchange<U8, U8>(rhs.numLods, 0);
        this->vertexIndexSpacePerLod = std::exchange(rhs.vertexIndexSpacePerLod, {});

        return *this;
    }
} // namespace ig
