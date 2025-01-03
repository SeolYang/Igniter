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

            GpuStorage::Allocation StorageSpace{}; // 아마 2개.. 일듯..? 서로다른 local frame 의 gpu storage 로 부터 받았기때문에
            U32 DataHashValue = IG_NUMERIC_MAX_OF(DataHashValue);
            GpuDataType GpuData{}; // 얘도 아마 2개로 유지해야할수도
            bool bMightBeDestroyed = false;
        };

        struct MaterialGpuData
        {
            U32 DiffuseTextureShaderResourceViewSlot = IG_NUMERIC_MAX_OF(DiffuseTextureShaderResourceViewSlot);
        };

        struct RenderableGpuData
        {
            U32 TransformStorageIndex = IG_NUMERIC_MAX_OF(TransformStorageIndex);
            U32 MaterialStorageIndex = IG_NUMERIC_MAX_OF(MaterialStorageIndex);
            U32 VertexOffset = 0u;
            U32 NumVertices = 0u;
            U32 IndexOffset = 0u;
            U32 NumIndices = 0u;
        };

        struct LightGpuData
        {
        };

        using TransformProxy = GpuProxy<Matrix>;
        using MaterialProxy = GpuProxy<MaterialGpuData>;
        using RenderableProxy = GpuProxy<RenderableGpuData>;
        using LightProxy = GpuProxy<LightGpuData>;

        template <typename Proxy, typename Owner = Entity>
        struct ProxyPackage
        {
            using ProxyMapType = UnorderedMap<Owner, Proxy>;

            InFlightFramesResource<Ptr<GpuStorage>> Storage;
            InFlightFramesResource<ProxyMapType> ProxyMap;
            Vector<Owner> PendingReplications;
            Vector<Owner> PendingDestructions;
        };

    public:
        explicit SceneProxy(RenderContext& renderContext, const MeshStorage& meshStorage, const AssetManager& assetManager);
        SceneProxy(const SceneProxy&) = delete;
        SceneProxy(SceneProxy&&) noexcept = delete;
        ~SceneProxy();

        SceneProxy& operator=(const SceneProxy&) = delete;
        SceneProxy& operator=(SceneProxy&&) noexcept = delete;

        // 여기서 렌더링 전 필요한 Scene 정보를 모두 모으고, GPU 메모리에 변경점 들을 반영해주어야 한다
        GpuSyncPoint Replicate(const LocalFrameIndex localFrameIdx, const World& world);

    private:
        void UpdateTransformProxy(const LocalFrameIndex localFrameIdx, const Registry& registry);
        void UpdateMaterialProxy(const LocalFrameIndex localFrameIdx, const Registry& registry);
        void UpdateRenderableProxy(const LocalFrameIndex localFrameIdx, const Registry& registry);
        void UpdateLightProxy(const LocalFrameIndex localFrameIdx, const Registry& registry);

        void PrepareStagingBuffer(const LocalFrameIndex localFrameIdx, const Size requiredSize);

        template <typename Proxy, typename Owner>
        void ReplicatePrxoyData(const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage,
                                CommandList& cmdList,
                                const Size stagingBufferOffset, const Size replicationSize);

        void ReplicateRenderableIndices(const LocalFrameIndex localFrameIdx,
                                        CommandList& cmdList,
                                        const Size stagingBufferOffset, const Size replicationSize);
        void UpdateLightIndicesBuffer(const LocalFrameIndex localFrameIdx);

    private:
        RenderContext* renderContext = nullptr;
        const AssetManager* assetManager = nullptr;
        const MeshStorage* meshStorage = nullptr;

        InFlightFramesResource<RenderHandle<GpuBuffer>> stagingBuffer;
        InFlightFramesResource<U8*> mappedStagingBuffer;
        InFlightFramesResource<Size> stagingBufferSize;

        constexpr static U32 kNumInitTransformElements = 1024u;
        ProxyPackage<TransformProxy> transformProxyPackage;

        constexpr static U32 kNumInitMaterialElements = 128u;
        ProxyPackage<MaterialProxy, ManagedAsset<Material>> materialProxyPackage;

        constexpr static U32 kNumInitRenderableElements = kNumInitTransformElements;
        ProxyPackage<RenderableProxy> renderableProxyPackage;

        constexpr static U32 kNumInitLightElements = 512u;
        ProxyPackage<LightProxy> lightProxyPackage;

        /* 현재 버퍼가 수용 할 수 있는 최대 수, 만약 부족하다면 새로 할당 필요! */
        constexpr static U32 kNumInitRenderableIndices = kNumInitRenderableElements;
        InFlightFramesResource<U32> renderableIndicesBufferSize;
        InFlightFramesResource<U32> numMaxRenderables;
        InFlightFramesResource<RenderHandle<GpuBuffer>> renderableIndicesBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> renderableIndicesBufferSrv;
        Vector<U32> renderableIndices;

        constexpr static U32 kNumInitLightIndices = 2048u;
        InFlightFramesResource<U32> lightIndicesBufferSize;
        InFlightFramesResource<U32> numMaxLights;
        InFlightFramesResource<RenderHandle<GpuBuffer>> lightIndicesBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> lightIndicesBufferSrv;
        Vector<U32> lightIndices;
    };
} // namespace ig
