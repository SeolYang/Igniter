#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/BoundingVolume.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/Light.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/Asset/StaticMesh.h"

namespace ig
{
    class GpuBuffer;
    class World;
    class Material;
    class CommandList;
    class MeshStorage;
    class GpuStagingBuffer;

    class SceneProxy
    {
    public:
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

        using MeshProxy = GpuProxy<GpuMesh>;
        using MeshInstanceProxy = GpuProxy<GpuMeshInstance>;
        using MaterialProxy = GpuProxy<GpuMaterial>;
        using LightProxy = GpuProxy<GpuLight>;

        template <typename Proxy, typename Owner = Entity>
        struct ProxyPackage
        {
        public:
            ProxyPackage(RenderContext& renderContext, const GpuStorageDesc& storageDesc, const Size numWorkers)
                : Storage(MakePtr<GpuStorage>(renderContext, storageDesc))
            {
                IG_CHECK(numWorkers > 0);
                PendingReplicationGroups.resize(numWorkers);
                PendingProxyGroups.resize(numWorkers);
                WorkGroupStagingBufferRanges.resize(numWorkers);
                WorkGroupCmdLists.resize(numWorkers);
            }

            ProxyPackage(const ProxyPackage&) = delete;
            ProxyPackage(ProxyPackage&&) noexcept = delete;

            ~ProxyPackage()
            {
                if (Storage != nullptr)
                {
                    Storage->ForceReset();
                }
            }

            ProxyPackage& operator=(const ProxyPackage&) = delete;
            ProxyPackage& operator=(ProxyPackage&&) noexcept = delete;

        public:
            using ProxyMapType = UnorderedMap<Owner, Proxy>;

            Ptr<GpuStorage> Storage{};
            ProxyMapType ProxyMap{};
            Vector<Owner> PendingDestructions{};

            InFlightFramesResource<Ptr<GpuBuffer>> StagingBuffer{};
            InFlightFramesResource<U8*> MappedStagingBuffer{};
            InFlightFramesResource<Size> StagingBufferSize{};

            Vector<Vector<std::pair<Owner, Proxy>>> PendingProxyGroups{};
            Vector<Vector<Owner>> PendingReplicationGroups{};

            // Temporary per replication!
            Vector<std::pair<Size, Size>> WorkGroupStagingBufferRanges{}; // <Offset, Size>
            Vector<CommandList*> WorkGroupCmdLists{};
        };

    public:
        explicit SceneProxy(tf::Executor& taskExecutor, RenderContext& renderContext, AssetManager& assetManager);
        SceneProxy(const SceneProxy&) = delete;
        SceneProxy(SceneProxy&&) noexcept = delete;
        ~SceneProxy();

        SceneProxy& operator=(const SceneProxy&) = delete;
        SceneProxy& operator=(SceneProxy&&) noexcept = delete;

        // 여기서 렌더링 전 필요한 Scene 정보를 모두 모으고, GPU 메모리에 변경점 들을 반영해주어야 한다
        [[nodiscard]] GpuSyncPoint Replicate(const LocalFrameIndex localFrameIdx, const World& world);
        void PrepareNextFrame(const LocalFrameIndex localFrameIdx);

        [[nodiscard]] Handle<GpuView> GetMaterialProxyStorageSrv() const
        {
            return materialProxyPackage.Storage->GetSrv();
        }

        [[nodiscard]] Handle<GpuView> GetLightStorageSrv() const
        {
            return lightProxyPackage.Storage->GetSrv();
        }

        [[nodiscard]] Handle<GpuBuffer> GetLightStorageBuffer() const
        {
            return lightProxyPackage.Storage->GetGpuBuffer();
        }

        [[nodiscard]] Handle<GpuView> GetStaticMeshProxyStorageSrv() const { return staticMeshProxyPackage.Storage->GetSrv(); }
        [[nodiscard]] Handle<GpuView> GetMeshInstanceProxyStorageSrv() const { return meshInstanceProxyPackage.Storage->GetSrv(); }
        [[nodiscard]] Handle<GpuView> GetMeshInstanceIndicesBufferSrv() const { return meshInstanceIndicesBufferSrv; }

        [[nodiscard]] U32 GetNumMeshInstances() const noexcept { return numMeshInstances; }
        [[nodiscard]] U16 GetNumLights() const noexcept { return (U16)lightProxyPackage.ProxyMap.size(); }

        [[nodiscard]] const auto& GetLightProxyMap() const noexcept { return lightProxyPackage.ProxyMap; }

    private:
        void UpdateLightProxy(tf::Subflow& subflow, const Registry& registry);
        void UpdateMaterialProxy();
        void UpdateStaticMeshProxy(tf::Subflow& subflow);
        void UpdateSkeletalMeshProxy(tf::Subflow& subflow);
        void UpdateMeshInstanceProxy(tf::Subflow& subflow, const Registry& registry);

        template <typename Proxy, typename Owner>
        void ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage);

        void ResizeMeshInstanceIndicesBuffer(const Size numMeshIndices);
        void UploadMeshInstanceIndices(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx);

    private:
        tf::Executor* taskExecutor = nullptr;
        RenderContext* renderContext = nullptr;
        AssetManager* assetManager = nullptr;

        InFlightFramesResource<tf::Future<void>> invalidationFuture;

        U32 numWorkers{1};

        constexpr static U32 kNumInitLightElements = kMaxNumLights;
        ProxyPackage<LightProxy> lightProxyPackage;
        constexpr static U32 kNumInitMaterialElements = 128u;
        ProxyPackage<MaterialProxy, Handle<Material>> materialProxyPackage;
        constexpr static U32 kNumInitMeshProxies = 512u;
        ProxyPackage<MeshProxy, Handle<StaticMesh>> staticMeshProxyPackage;
        //ProxyPackage<MeshProxy, Handle<class SkeletalMesh>> skeletalMeshProxyPackage;

        ProxyPackage<MeshInstanceProxy> meshInstanceProxyPackage;

        constexpr static Size kInitNumMeshInstanceIndices = 16'384;
        Vector<Vector<U32>> meshInstanceIndicesGroups;
        U32 numMeshInstances{0};
        /* First: OffsetBytes, Second: MeshInstanceIndicesGroupsIdx */
        Vector<std::pair<U32, Index>> meshInstanceIndicesUploadInfos;
        Bytes meshInstanceIndicesBufferSize = 0;
        Handle<GpuBuffer> meshInstanceIndicesBuffer{};
        Handle<GpuView> meshInstanceIndicesBufferSrv{};
        Ptr<GpuStagingBuffer> meshInstanceIndicesStagingBuffer{nullptr};
    };
} // namespace ig
