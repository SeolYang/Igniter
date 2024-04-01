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