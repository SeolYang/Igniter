#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/Light.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/StaticMesh.h"

namespace ig
{
    class GpuBuffer;
    class World;
    class Material;
    class CommandList;
    class MeshStorage;
    class SceneProxy
    {
      public:
        enum class ERenderableType : U32
        {
            StaticMesh,
            SkeletalMesh, // Reserved for future!
            VolumetricFog,
            ParticleEmitter,
        };

        // 애초에 GpuProxy, 즉 Entity-Proxy Map을 두개로 관리하면?
        template <typename GpuDataType>
        struct GpuProxy
        {
          public:
            struct UploadInfo
            {
                CRef<GpuStorage::Allocation> StorageSpaceRef;
                CRef<GpuDataType> GpuDataRef;
            };

            using GpuData_t = GpuDataType;

          public:
            constexpr static U32 kDataSize = (U32)sizeof(GpuDataType);

            GpuStorage::Allocation StorageSpace{};
            U64 DataHashValue = IG_NUMERIC_MAX_OF(DataHashValue);
            GpuDataType GpuData{};
            bool bMightBeDestroyed = false;
        };

        struct TransformGpuData
        {
            // HLSL에선 Row-Vector로 받아들이기 때문에 해당 벡터로 행렬을 만들고 전치 해주어야 한다.
            Vector4 Cols[3];
        };

        struct MaterialGpuData
        {
            U32 DiffuseTextureSrv = IG_NUMERIC_MAX_OF(DiffuseTextureSrv);
            U32 DiffuseTextureSampler = IG_NUMERIC_MAX_OF(DiffuseTextureSampler);
        };

        struct MeshLodGpuData
        {
            U32 IndexOffset = 0u;
            U32 NumIndices = 0u;
        };

        struct MeshGpuData
        {
            U32 VertexOffset = 0u;
            U32 NumVertices = 0u;

            U32 NumLods = 0u;
            Array<MeshLodGpuData, StaticMesh::kMaxNumLods> Lods;
            BoundingSphere BoundingVolume;
        };

        struct RenderableGpuData
        {
            ERenderableType Type;
            U32 DataIdx = IG_NUMERIC_MAX_OF(DataIdx);
            U32 TransformDataIdx = IG_NUMERIC_MAX_OF(TransformDataIdx);
        };

        struct LightGpuData
        {
            Light Property;
            Vector3 WorldPosition;
            Vector3 Forward;
        };

        using TransformProxy = GpuProxy<TransformGpuData>;
        using MaterialProxy = GpuProxy<MaterialGpuData>;
        using MeshProxy = GpuProxy<MeshGpuData>;
        using RenderableProxy = GpuProxy<RenderableGpuData>;
        using LightProxy = GpuProxy<LightGpuData>;

        template <typename Proxy, typename Owner = Entity>
        struct ProxyPackage
        {
            using ProxyMapType = UnorderedMap<Owner, Proxy>;

            Ptr<GpuStorage> Storage;
            ProxyMapType ProxyMap;
            Vector<Owner> PendingDestructions; // Deprecated

            InFlightFramesResource<Ptr<GpuBuffer>> StagingBuffer;
            InFlightFramesResource<U8*> MappedStagingBuffer;
            InFlightFramesResource<Size> StagingBufferSize;

            Vector<Vector<std::pair<Owner, Proxy>>> PendingProxyGroups;
            Vector<Vector<Owner>> PendingReplicationGroups;
            // Vector<Vector<Owner>> PendingDestructionGroups;

            // Temporary per replication!
            Vector<std::pair<Size, Size>> WorkGroupStagingBufferRanges; // <Offset, Size>
            Vector<CommandList*> WorkGroupCmdLists;
        };

        struct InstancingGpuData
        {
            U32 MaterialDataIdx = IG_NUMERIC_MAX_OF(MaterialDataIdx);
            U32 MeshDataIdx = IG_NUMERIC_MAX_OF(MeshDataIdx);
            U32 InstancingId = IG_NUMERIC_MAX_OF(InstancingId);
            // [IndirectTransformOffset, IndirectTransformOffset + NumInstances)
            U32 IndirectTransformOffset = IG_NUMERIC_MAX_OF(IndirectTransformOffset);
            U32 NumInstances = 0;
            Array<U32, StaticMesh::kMaxNumLods> NumVisibleLodInstances{0};
        };

        using InstancingMap = UnorderedMap<U64, InstancingGpuData>;
        using OrderedInstancingMap = OrderedMap<U64, InstancingGpuData>;
        struct InstancingPackage
        {
          public:
            static inline U64 MakeInstancingMapKey(const MeshProxy& meshProxy, const MaterialProxy& materialProxy)
            {
                return ((U64)materialProxy.StorageSpace.OffsetIndex << 32) | meshProxy.StorageSpace.OffsetIndex;
            }

          public:
            Ptr<GpuStorage> InstancingDataStorage;
            Ptr<GpuStorage> IndirectTransformStorage;
            Vector<InstancingMap> ThreadLocalInstancingMaps;
            OrderedInstancingMap GlobalInstancingMap;
            U32 NumInstances{0};
            GpuStorage::Allocation InstancingDataSpace;
            GpuStorage::Allocation IndirectTransformSpace;

            InFlightFramesResource<Ptr<GpuBuffer>> StagingBuffer;
            InFlightFramesResource<U8*> MappedStagingBuffer;
            InFlightFramesResource<Size> StagingBufferSize;
        };

        struct StaticMeshRenderableData
        {
            Entity Owner = NullEntity;
            U64 InstancingKey = IG_NUMERIC_MAX_OF(InstancingKey);
        };
        Vector<Vector<StaticMeshRenderableData>> localIntermediateStaticMeshData;

      public:
        explicit SceneProxy(tf::Executor& taskExecutor, RenderContext& renderContext, const MeshStorage& meshStorage, AssetManager& assetManager);
        SceneProxy(const SceneProxy&) = delete;
        SceneProxy(SceneProxy&&) noexcept = delete;
        ~SceneProxy();

        SceneProxy& operator=(const SceneProxy&) = delete;
        SceneProxy& operator=(SceneProxy&&) noexcept = delete;

        // 여기서 렌더링 전 필요한 Scene 정보를 모두 모으고, GPU 메모리에 변경점 들을 반영해주어야 한다
        [[nodiscard]] GpuSyncPoint Replicate(const LocalFrameIndex localFrameIdx, const World& world);
        void PrepareNextFrame(const LocalFrameIndex localFrameIdx);
        
        [[nodiscard]] Handle<GpuView> GetTransformProxyStorageSrv() const
        {
            return transformProxyPackage.Storage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetMaterialProxyStorageSrv() const
        {
            return materialProxyPackage.Storage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetMeshProxySrv() const
        {
            return meshProxyPackage.Storage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuBuffer> GetInstancingStorageBuffer() const
        {
            return instancingPackage.InstancingDataStorage->GetGpuBuffer();
        }

        [[nodiscard]] Handle<GpuView> GetInstancingStorageSrv() const
        {
            return instancingPackage.InstancingDataStorage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetInstancingStorageUav() const
        {
            return instancingPackage.InstancingDataStorage->GetUnorderedResourceView();
        }

        [[nodiscard]] Handle<GpuBuffer> GetIndirectTransformStorageBuffer() const
        {
            return instancingPackage.IndirectTransformStorage->GetGpuBuffer();
        }

        [[nodiscard]] Handle<GpuView> GetIndirectTransformStorageSrv() const
        {
            return instancingPackage.IndirectTransformStorage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetIndirectTransformStorageUav() const
        {
            return instancingPackage.IndirectTransformStorage->GetUnorderedResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetRenderableProxyStorageSrv() const
        {
            return renderableProxyPackage.Storage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuView> GetRenderableIndicesSrv() const
        {
            return renderableIndicesBufferSrv;
        }

        [[nodiscard]] Handle<GpuView> GetLightStorageSrv() const
        {
            return lightProxyPackage.Storage->GetShaderResourceView();
        }

        [[nodiscard]] Handle<GpuBuffer> GetLightStorageBuffer() const
        {
            return lightProxyPackage.Storage->GetGpuBuffer();
        }

        [[nodiscard]] U32 GetNumRenderables() const noexcept
        {
            return (U32)renderableIndices.size();
        }

        [[nodiscard]] U32 GetNumInstancing() const noexcept { return (U32)instancingPackage.GlobalInstancingMap.size(); }

        [[nodiscard]] U16 GetNumLights() const noexcept { return (U16)lightProxyPackage.ProxyMap.size(); }

        [[nodiscard]] const auto& GetLightProxyMap() const noexcept { return lightProxyPackage.ProxyMap; }

      private:
        void UpdateMaterialProxy();
        void UpdateMeshProxy();
        void UpdateTransformProxy(tf::Subflow& subflow, const Registry& registry);
        void UpdateLightProxy(tf::Subflow& subflow, const Registry& registry);

        void BuildInstancingData(tf::Subflow& subflow,const Registry& registry);

        void UpdateRenderableProxy(tf::Subflow& subflow);

        template <typename Proxy, typename Owner>
        void ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage);

        void ReplicateInstancingData(const LocalFrameIndex localFrameIdx);

        void ReplicateRenderableIndices(const LocalFrameIndex localFrameIdx);

      private:
        tf::Executor* taskExecutor = nullptr;
        RenderContext* renderContext = nullptr;
        AssetManager* assetManager = nullptr;
        const MeshStorage* meshStorage = nullptr;

        InFlightFramesResource<tf::Future<void>> invalidationFuture;

        U32 numWorker{1};

        constexpr static U32 kNumInitTransformElements = 1024u;
        ProxyPackage<TransformProxy> transformProxyPackage;

        constexpr static U32 kNumInitMaterialElements = 128u;
        ProxyPackage<MaterialProxy, Handle<Material>> materialProxyPackage;

        constexpr static U32 kNumInitMeshElements = 256u;
        ProxyPackage<MeshProxy, Handle<StaticMesh>> meshProxyPackage;

        constexpr static U32 kNumInitStaticMeshInstances = 256u;
        InstancingPackage instancingPackage;

        constexpr static U32 kNumInitRenderableElements = 2048u;
        ProxyPackage<RenderableProxy> renderableProxyPackage;

        constexpr static U32 kNumInitLightElements = kMaxNumLights;
        ProxyPackage<LightProxy> lightProxyPackage;

        /* 현재 버퍼가 수용 할 수 있는 최대 수, 만약 부족하다면 새로 할당 필요! */
        constexpr static U32 kNumInitRenderableIndices = kNumInitRenderableElements;
        U32 renderableIndicesBufferSize;
        Handle<GpuBuffer> renderableIndicesBuffer;
        Handle<GpuView> renderableIndicesBufferSrv;
        Vector<Vector<U32>> renderableIndicesGroups;
        Vector<U32> renderableIndices;
        // #sy_todo 나중에 StagingBuffer로 묶어서 간략하게 리팩토링하기
        InFlightFramesResource<Handle<GpuBuffer>> renderableIndicesStagingBuffer;
        InFlightFramesResource<U8*> mappedRenderableIndicesStagingBuffer;
        InFlightFramesResource<Size> renderableIndicesStagingBufferSize;
    };
} // namespace ig
