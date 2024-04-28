#include <Igniter.h>
#include <Core/Json.h>
#include <Core/Engine.h>
#include <D3D12/GpuBuffer.h>
#include <Render/RenderContext.h>
#include <Asset/AssetManager.h>
#include <Asset/StaticMesh.h>

namespace ig
{
    Json& StaticMeshImportDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bMakeLeftHanded);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateNormals);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bSplitLargeMeshes);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bPreTransformVertices);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bImproveCacheLocality);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateUVCoords);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bFlipUVs);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bFlipWindingOrder);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateBoundingBoxes);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bImportMaterials);
        return archive;
    }

    const Json& StaticMeshImportDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bMakeLeftHanded, bMakeLeftHanded);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bFlipUVs, bFlipUVs);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bFlipWindingOrder, bFlipWindingOrder);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateNormals, bGenerateNormals);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bSplitLargeMeshes, bSplitLargeMeshes);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bPreTransformVertices, bPreTransformVertices);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bImproveCacheLocality, bImproveCacheLocality);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateUVCoords, bGenerateUVCoords);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bGenerateBoundingBoxes, bGenerateBoundingBoxes);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshImportDesc, archive, bImportMaterials, bImportMaterials);
        return archive;
    }

    Json& StaticMeshLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, NumVertices);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, NumIndices);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, CompressedIndicesSizeInBytes);
        IG_SERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, MaterialGuid);
        return archive;
    }

    const Json& StaticMeshLoadDesc::Deserialize(const Json& archive)
    {
        *this = {};
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, NumVertices, NumVertices);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, NumIndices, NumIndices);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, CompressedVerticesSizeInBytes, CompressedVerticesSizeInBytes);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, CompressedIndicesSizeInBytes, CompressedIndicesSizeInBytes);
        IG_DESERIALIZE_JSON_SIMPLE(StaticMeshLoadDesc, archive, MaterialGuid, MaterialGuid);
        return archive;
    }

    StaticMesh::StaticMesh(RenderContext& renderContext, AssetManager& assetManager, const Desc& snapshot, const Handle<GpuBuffer> vertexBuffer,
        const Handle<GpuView> vertexBufferSrv, const Handle<GpuBuffer> indexBuffer, const ManagedAsset<Material> material)
        : renderContext(&renderContext)
        , assetManager(&assetManager)
        , snapshot(snapshot)
        , vertexBuffer(vertexBuffer)
        , vertexBufferSrv(vertexBufferSrv)
        , indexBuffer(indexBuffer)
        , material(material)
    {
    }

    StaticMesh::~StaticMesh()
    {
        if (assetManager != nullptr)
        {
            assetManager->Unload(material);
        }

        if (renderContext != nullptr)
        {
            renderContext->DestroyBuffer(vertexBuffer);
            renderContext->DestroyBuffer(indexBuffer);
            renderContext->DestroyGpuView(vertexBufferSrv);
        }
    }
}    // namespace ig
