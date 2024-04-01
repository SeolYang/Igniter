#include <Igniter.h>
#include <Core/JsonUtils.h>
#include <Core/Serializable.h>
#include <D3D12/GpuBuffer.h>
#include <Render/GpuViewManager.h>
#include <Asset/StaticMesh.h>

namespace ig
{
    json& StaticMeshImportDesc::Serialize(json& archive) const
    {
        const auto& desc = *this;
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bMakeLeftHanded);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateNormals);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bSplitLargeMeshes);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bPreTransformVertices);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bImproveCacheLocality);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateUVCoords);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bFlipUVs);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bFlipWindingOrder);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateBoundingBoxes);
        IG_SERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bImportMaterials);
        return archive;
    }

    const json& StaticMeshImportDesc::Deserialize(const json& archive)
    {
        auto& desc = *this;
        desc = {};
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bMakeLeftHanded, desc.bMakeLeftHanded);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bFlipUVs, desc.bFlipUVs);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bFlipWindingOrder, desc.bFlipWindingOrder);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateNormals, desc.bGenerateNormals);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bSplitLargeMeshes, desc.bSplitLargeMeshes);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bPreTransformVertices, desc.bPreTransformVertices);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bImproveCacheLocality, desc.bImproveCacheLocality);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateUVCoords, desc.bGenerateUVCoords);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bGenerateBoundingBoxes, desc.bGenerateBoundingBoxes);
        IG_DESERIALIZE_JSON(StaticMeshImportDesc, archive, desc, bImportMaterials, desc.bImportMaterials);
        return archive;
    }

    json& StaticMeshLoadDesc::Serialize(json& archive) const
    {
        const auto& desc = *this;
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, NumVertices);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, NumIndices);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, CompressedVerticesSizeInBytes);
        IG_SERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, CompressedIndicesSizeInBytes);
        return archive;
    }

    const json& StaticMeshLoadDesc::Deserialize(const json& archive)
    {
        auto& desc = *this;
        desc = {};
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, NumVertices, desc.NumVertices);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, NumIndices, desc.NumIndices);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, CompressedVerticesSizeInBytes, desc.CompressedVerticesSizeInBytes);
        IG_DESERIALIZE_JSON(StaticMeshLoadDesc, archive, desc, CompressedIndicesSizeInBytes, desc.CompressedIndicesSizeInBytes);
        return archive;
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