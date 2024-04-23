#include <Igniter.h>
#include <Core/Json.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
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

    StaticMesh::StaticMesh(const Desc& snapshot, DeferredHandle<GpuBuffer> vertexBuffer, Handle<GpuView, GpuViewManager*> vertexBufferSrv,
        DeferredHandle<GpuBuffer> indexBuffer, CachedAsset<Material> material)
        : snapshot(snapshot)
        , vertexBuffer(std::move(vertexBuffer))
        , vertexBufferSrv(std::move(vertexBufferSrv))
        , indexBuffer(std::move(indexBuffer))
        , material(std::move(material))
    {
        IG_CHECK(this->vertexBuffer);
        IG_CHECK(this->vertexBufferSrv);
        IG_CHECK(this->indexBuffer);
        IG_CHECK(this->material);
    }

    StaticMesh::~StaticMesh() {}
}    // namespace ig
