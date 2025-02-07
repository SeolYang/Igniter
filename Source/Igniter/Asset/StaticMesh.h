#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/Material.h"

namespace ig
{
    class StaticMesh;
    struct StaticMeshImportDesc
    {
      public:
        constexpr static U8 kMaxNumLods = 8;

      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

      public:
        bool bMakeLeftHanded = true;
        bool bFlipUVs = true;
        bool bFlipWindingOrder = true;
        bool bGenerateNormals = false;
        bool bSplitLargeMeshes = false;
        bool bPreTransformVertices = false;
        bool bGenerateUVCoords = false;
        bool bGenerateBoundingBoxes = false;
        bool bImproveCacheLocality = false;
        bool bImportMaterials = false; /* Only if materials does not exist or not imported before. */

        bool bGenerateLODs = true;
        // 1 <= NumLods <= kMaxNumLods, bGenerateLods && 2 <= NumLods => Generate New Lods
        U8 NumLods = 1;
        // LOD (NumLods-1) 에서 보존될 인덱스 수에 대한 계수
        // 값에 따라 최소 (1.0-MaxSimplificationFactor) * (NumIndices[LOD0])까지 타겟 인덱스 수를 설정 한다.
        F32 MaxSimplificationFactor = 0.5f;

        bool bOptimizeVertexCache = true;
        bool bOptimizeOverdraw = true;
        /* Overdraw Optimization으로 인해 생길 수 있는 정점 캐시 효율 하락을 얼마만큼 허용 할 지(1.05 == 5%) */
        F32 OverdrawOptimizerThreshold = 1.05f;
        bool bOptimizeVertexFetch = true;
    };

    struct StaticMeshLoadDesc
    {
      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

      public:
        U32 NumVertices{0};
        Size CompressedVerticesSizeInBytes{0};

        U8 NumLods{1};
        Array<U32, StaticMeshImportDesc::kMaxNumLods> NumIndicesPerLod;
        Array<Size, StaticMeshImportDesc::kMaxNumLods> CompressedIndicesSizePerLod;
        AABB AABB{};
    };

    class GpuBuffer;
    class GpuView;
    class Material;
    class RenderContext;
    class AssetManager;
    class StaticMesh final
    {
      public:
        constexpr static U8 kMaxNumLods = StaticMeshImportDesc::kMaxNumLods;

      public:
        using ImportDesc = StaticMeshImportDesc;
        using LoadDesc = StaticMeshLoadDesc;
        using Desc = AssetDesc<StaticMesh>;

      public:
        StaticMesh(
            RenderContext& renderContext, AssetManager& assetManager,
            const Desc& snapshot,
            const Handle<MeshStorage::Space<VertexSM>> vertexSpace,
            const U8 numLods,
            const Array<Handle<MeshStorage::Space<U32>>, kMaxNumLods> vertexIndexSpacePerLod);

        StaticMesh(const StaticMesh&) = delete;
        StaticMesh(StaticMesh&& other) noexcept;
        ~StaticMesh();

        StaticMesh& operator=(const StaticMesh&) = delete;
        StaticMesh& operator=(StaticMesh&& rhs) noexcept;

        [[nodiscard]] const Desc& GetSnapshot() const { return snapshot; }
        [[nodiscard]] Handle<MeshStorage::Space<VertexSM>> GetVertexSpace() const noexcept { return vertexSpace; }
        [[nodiscard]] U8 GetNumLods() const noexcept { return numLods; }
        [[nodiscard]] Handle<MeshStorage::Space<U32>> GetIndexSpace(const U8 lod) const noexcept
        {
            if (lod >= numLods)
            {
                return {};
            }

            return vertexIndexSpacePerLod[lod];
        }

      private:
        void Destroy();

      private:
        RenderContext* renderContext{nullptr};
        AssetManager* assetManager{nullptr};
        Desc snapshot{}; // desc snapshot

        Handle<MeshStorage::Space<VertexSM>> vertexSpace{};

        U8 numLods = 1; // least 1
        Array<Handle<MeshStorage::Space<U32>>, kMaxNumLods> vertexIndexSpacePerLod;
    };
} // namespace ig
