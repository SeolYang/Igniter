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
        return archive;
    }

    Json& StaticMeshLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumVertices);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumIndices);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, CompressedIndicesSizeInBytes);

        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, AABB);

        return archive;
    }

    const Json& StaticMeshLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumVertices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumIndices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, CompressedIndicesSizeInBytes);

        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, AABB);
        return archive;
    }

    StaticMesh::StaticMesh(RenderContext& renderContext, AssetManager& assetManager,
                           const Desc& snapshot,
                           const MeshStorage::Handle<VertexSM> vertexSpace, MeshStorage::Handle<U32> vertexIndexSpace)
        : renderContext(&renderContext)
        , assetManager(&assetManager)
        , snapshot(snapshot)
        , vertexSpace(vertexSpace)
        , vertexIndexSpace(vertexIndexSpace)
    {
    }

    StaticMesh::~StaticMesh()
    {
        Destroy();
    }

    void StaticMesh::Destroy()
    {
        MeshStorage& meshStorage = Engine::GetMeshStorage();
        meshStorage.Destroy(vertexSpace);
        meshStorage.Destroy(vertexIndexSpace);

        renderContext = nullptr;
        assetManager = nullptr;
        vertexSpace = {};
        vertexIndexSpace = {};
    }

    StaticMesh& StaticMesh::operator=(StaticMesh&& rhs) noexcept
    {
        Destroy();

        this->renderContext = std::exchange(rhs.renderContext, nullptr);
        this->assetManager = std::exchange(rhs.assetManager, nullptr);
        this->snapshot = rhs.snapshot;
        this->vertexSpace = std::exchange(rhs.vertexSpace, {});
        this->vertexIndexSpace = std::exchange(rhs.vertexIndexSpace, {});

        return *this;
    }

} // namespace ig
