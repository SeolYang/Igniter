#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Asset/Common.h"

namespace ig
{
    class GpuBuffer;
    class World;
    class Material;
    class SceneProxy
    {
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
            bool bMightBeDestroyed = true;
        };

        struct MaterialGpuData
        {
            U32 DiffuseTextureSrv = IG_NUMERIC_MAX_OF(DiffuseTextureSrv);
            U32 DiffuseTextureSampler = IG_NUMERIC_MAX_OF(DiffuseTextureSampler);
        };

        struct StaticMeshGpuData
        {
            U32 MaterialDataIdx = IG_NUMERIC_MAX_OF(MaterialDataIdx);
            U32 VertexOffset = 0u;
            U32 NumVertices = 0u;
            U32 IndexOffset = 0u;
            U32 NumIndices = 0u;
        };

        struct RenderableGpuData
        {
            ERenderableType Type;
            U32 DataIdx;
            U32 TransformDataIdx = IG_NUMERIC_MAX_OF(TransformDataIdx);
        };

        struct LightGpuData
        {
        };

        using TransformProxy = GpuProxy<Matrix>;
        using MaterialProxy = GpuProxy<MaterialGpuData>;
        using StaticMeshProxy = GpuProxy<StaticMeshGpuData>;
        using RenderableProxy = GpuProxy<RenderableGpuData>;
        using LightProxy = GpuProxy<LightGpuData>;

        template <typename Proxy, typename Owner = Entity>
        struct ProxyPackage
        {
            using ProxyMapType = UnorderedMap<Owner, Proxy>;

            InFlightFramesResource<Ptr<GpuStorage>> Storage;
            InFlightFramesResource<ProxyMapType> ProxyMap;
            Vector<Owner> PendingDestructions; // Deprecated

            InFlightFramesResource<Ptr<GpuBuffer>> StagingBuffer;
            InFlightFramesResource<U8*> MappedStagingBuffer;
            InFlightFramesResource<Size> StagingBufferSize;

            Vector<Vector<std::pair<Owner, Proxy>>> PendingProxyGroups;
            Vector<Vector<Owner>> PendingReplicationGroups;
            //Vector<Vector<Owner>> PendingDestructionGroups;

            // Temporary per replication!
            Vector<std::pair<Size, Size>> WorkGroupStagingBufferRanges; // <Offset, Size>
            Vector<CommandList*> WorkGroupCmdLists;
            Size NumValidWorkGroupCmdList{0};
        };

    public:
        explicit SceneProxy(RenderContext& renderContext, const MeshStorage& meshStorage, AssetManager& assetManager);
        SceneProxy(const SceneProxy&) = delete;
        SceneProxy(SceneProxy&&) noexcept = delete;
        ~SceneProxy();

        SceneProxy& operator=(const SceneProxy&) = delete;
        SceneProxy& operator=(SceneProxy&&) noexcept = delete;

        // 여기서 렌더링 전 필요한 Scene 정보를 모두 모으고, GPU 메모리에 변경점 들을 반영해주어야 한다
        [[nodiscard]] GpuSyncPoint Replicate(const LocalFrameIndex localFrameIdx, const World& world);

        [[nodiscard]] RenderHandle<GpuView> GetTransformStorageShaderResourceView(const LocalFrameIndex localFrameIdx) const
        {
            return transformProxyPackage.Storage[localFrameIdx]->GetShaderResourceView();
        }

        [[nodiscard]] RenderHandle<GpuView> GetMaterialStorageShaderResourceView(const LocalFrameIndex localFrameIdx) const
        {
            return materialProxyPackage.Storage[localFrameIdx]->GetShaderResourceView();
        }

        [[nodiscard]] RenderHandle<GpuView> GetStaticMeshStorageSrv(const LocalFrameIndex localFrameIdx) const
        {
            return staticMeshProxyPackage.Storage[localFrameIdx]->GetShaderResourceView();
        }

        [[nodiscard]] RenderHandle<GpuView> GetRenderableStorageShaderResourceView(const LocalFrameIndex localFrameIdx) const
        {
            return renderableProxyPackage.Storage[localFrameIdx]->GetShaderResourceView();
        }

        [[nodiscard]] RenderHandle<GpuView> GetRenderableIndicesBufferShaderResourceView(const LocalFrameIndex localFrameIdx) const
        {
            return renderableIndicesBufferSrv[localFrameIdx];
        }

        [[nodiscard]] U32 GetNumMaxRenderables(const LocalFrameIndex localFrameIdx) const noexcept
        {
            return numMaxRenderables[localFrameIdx];
        }

    private:
        void UpdateMaterialProxy(const LocalFrameIndex localFrameIdx);
        void UpdateTransformProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, const Registry& registry);
        void UpdateStaticMeshProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, const Registry& registry);
        void UpdateRenderableProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx);

        void UpdateLightEntityProxy(const LocalFrameIndex localFrameIdx, const Registry& registry);

        template <typename Proxy, typename Owner>
        void ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage);

        void ReplicateRenderableIndices(const LocalFrameIndex localFrameIdx);
        void ReplicateLightIndicesBuffer(const LocalFrameIndex localFrameIdx);

    private:
        RenderContext* renderContext = nullptr;
        AssetManager* assetManager = nullptr;
        const MeshStorage* meshStorage = nullptr;

        U32 numWorker{1};

        constexpr static U32 kNumInitTransformElements = 1024u;
        ProxyPackage<TransformProxy> transformProxyPackage;

        constexpr static U32 kNumInitMaterialElements = 128u;
        ProxyPackage<MaterialProxy, ManagedAsset<Material>> materialProxyPackage;

        constexpr static U32 kNumInitStaticMeshElements = 256u;
        ProxyPackage<StaticMeshProxy> staticMeshProxyPackage;

        constexpr static U32 kNumInitRenderableElements = 2048u;
        ProxyPackage<RenderableProxy> renderableProxyPackage;

        constexpr static U32 kNumInitLightElements = 512u;
        ProxyPackage<LightProxy> lightProxyPackage;

        /* 현재 버퍼가 수용 할 수 있는 최대 수, 만약 부족하다면 새로 할당 필요! */
        constexpr static U32 kNumInitRenderableIndices = kNumInitRenderableElements;
        InFlightFramesResource<U32> renderableIndicesBufferSize;
        InFlightFramesResource<U32> numMaxRenderables;
        InFlightFramesResource<RenderHandle<GpuBuffer>> renderableIndicesBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> renderableIndicesBufferSrv;
        Vector<Vector<U32>> renderableIndicesGroups;
        Vector<U32> renderableIndices;
        // #sy_todo 나중에 StagingBuffer로 묶어서 간략하게 리팩토링하기
        InFlightFramesResource<RenderHandle<GpuBuffer>> renderableIndicesStagingBuffer;
        InFlightFramesResource<U8*> mappedRenderableIndicesStagingBuffer;
        InFlightFramesResource<Size> renderableIndicesStagingBufferSize;


        constexpr static U32 kNumInitLightIndices = 2048u;
        InFlightFramesResource<U32> lightIndicesBufferSize;
        InFlightFramesResource<U32> numMaxLights;
        InFlightFramesResource<RenderHandle<GpuBuffer>> lightIndicesBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> lightIndicesBufferSrv;
        Vector<U32> lightIndices;
    };
} // namespace ig
