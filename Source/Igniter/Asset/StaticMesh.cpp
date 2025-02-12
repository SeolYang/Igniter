#include "Igniter/Igniter.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/UnifiedMeshStorage.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/StaticMesh.h"

namespace ig
{
    Json& StaticMeshImportDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bMakeLeftHanded);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bSplitLargeMeshes);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bPreTransformVertices);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bImproveCacheLocality);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateUVCoords);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bFlipUVs);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bFlipWindingOrder);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateBoundingBoxes);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bImportMaterials);
        IG_SERIALIZE_TO_JSON(StaticMeshImportDesc, archive, bGenerateLODs);
        return archive;
    }

    const Json& StaticMeshImportDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bMakeLeftHanded);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bFlipUVs);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bFlipWindingOrder);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bSplitLargeMeshes);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bPreTransformVertices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bImproveCacheLocality);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateUVCoords);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateBoundingBoxes);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bImportMaterials);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshImportDesc, archive, bGenerateLODs);
        return archive;
    }

    Json& StaticMeshLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumVertices);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, CompressedVerticesSize);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumLevelOfDetails);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumMeshletVertexIndices);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumMeshletTriangles);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, NumMeshlets);
        IG_SERIALIZE_TO_JSON(StaticMeshLoadDesc, archive, BoundingBox);
        return archive;
    }

    const Json& StaticMeshLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumVertices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, CompressedVerticesSize);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumLevelOfDetails);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumMeshletVertexIndices);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumMeshletTriangles);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, NumMeshlets);
        IG_DESERIALIZE_FROM_JSON_NO_FALLBACK(StaticMeshLoadDesc, archive, BoundingBox);
        return archive;
    }

    StaticMesh::StaticMesh(RenderContext& renderContext, AssetManager& assetManager, const Desc& snapshot, const Mesh& newMesh)
        : renderContext(&renderContext)
        , assetManager(&assetManager)
        , snapshot(snapshot)
        , mesh(newMesh)
    {
    }

    StaticMesh::StaticMesh(StaticMesh&& other) noexcept
        : renderContext(std::exchange(other.renderContext, nullptr))
        , assetManager(std::exchange(other.assetManager, nullptr))
        , snapshot(std::exchange(other.snapshot, {}))
        , mesh(std::exchange(other.mesh, {}))
    {
    }

    StaticMesh::~StaticMesh()
    {
        Destroy();
    }

    StaticMesh& StaticMesh::operator=(StaticMesh&& rhs) noexcept
    {
        renderContext = std::exchange(rhs.renderContext, nullptr);
        assetManager = std::exchange(rhs.assetManager, nullptr);
        snapshot = std::exchange(rhs.snapshot, {});
        mesh = std::exchange(rhs.mesh, {});
        return *this;
    }

    void StaticMesh::Destroy()
    {
        if (renderContext == nullptr)
        {
            return;
        }

        UnifiedMeshStorage& unifiedMeshStorae = renderContext->GetUnifiedMeshStorage();
        {
            unifiedMeshStorae.Deallocate(mesh.VertexStorageAlloc);
        }

        for (U8 lod = 0; lod < mesh.NumLevelOfDetails; ++lod)
        {
            MeshLod& meshLod = mesh.LevelOfDetails[lod];
            if (meshLod.IndexStorageAlloc)
            {
                unifiedMeshStorae.Deallocate(meshLod.IndexStorageAlloc);
            }
            if (meshLod.TriangleStorageAlloc)
            {
                unifiedMeshStorae.Deallocate(meshLod.TriangleStorageAlloc);
            }
            if (meshLod.MeshletStorageAlloc)
            {
                unifiedMeshStorae.Deallocate(meshLod.MeshletStorageAlloc);
            }
        }

        snapshot = {};
        mesh = {};
    }
} // namespace ig
