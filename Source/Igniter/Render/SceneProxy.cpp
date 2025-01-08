#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/Asset/StaticMesh.h"
#include "Igniter/Asset/Texture.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Render/SceneProxy.h"

namespace ig
{
    SceneProxy::SceneProxy(RenderContext& renderContext, const MeshStorage& meshStorage, AssetManager& assetManager) :
        renderContext(&renderContext),
        meshStorage(&meshStorage),
        assetManager(&assetManager)
    {
        GpuBufferDesc renderableIndicesBufferDesc;
        renderableIndicesBufferDesc.AsStructuredBuffer<U32>(kNumInitRenderableIndices);

        GpuBufferDesc lightIndicesBufferDesc;
        lightIndicesBufferDesc.AsStructuredBuffer<U32>(kNumInitLightIndices);

        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            transformProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("TransformStorage{}", localFrameIdx)),
                    TransformProxy::kDataSize,
                    kNumInitTransformElements);

            materialProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("MaterialDataStorage{}", localFrameIdx)),
                    MaterialProxy::kDataSize,
                    kNumInitMaterialElements);

            staticMeshProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("StaticMeshDataStorage{}", localFrameIdx)),
                    StaticMeshProxy::kDataSize,
                    kNumInitStaticMeshElements);

            renderableProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("RenderableStorage{}", localFrameIdx)),
                    RenderableProxy::kDataSize,
                    kNumInitRenderableElements);

            lightProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("LightDataStorage{}", localFrameIdx)),
                    LightProxy::kDataSize,
                    kNumInitLightElements);

            renderableIndicesBufferSize[localFrameIdx] = kNumInitRenderableIndices;
            renderableIndicesBufferDesc.DebugName = String(std::format("RenderableIndicesBuffer{}", localFrameIdx));
            renderableIndicesBuffer[localFrameIdx] = renderContext.CreateBuffer(renderableIndicesBufferDesc);
            renderableIndicesBufferSrv[localFrameIdx] = renderContext.CreateShaderResourceView(renderableIndicesBuffer[localFrameIdx]);

            lightIndicesBufferSize[localFrameIdx] = kNumInitLightIndices;
            lightIndicesBufferDesc.DebugName = String(std::format("LightIndicesBuffer{}", localFrameIdx));
            lightIndicesBuffer[localFrameIdx] = renderContext.CreateBuffer(lightIndicesBufferDesc);
            lightIndicesBufferSrv[localFrameIdx] = renderContext.CreateShaderResourceView(lightIndicesBuffer[localFrameIdx]);
        }

        const U32 numThreads = std::thread::hardware_concurrency();
        transformProxyPackage.PendingReplicationGroups.resize(numThreads);
        transformProxyPackage.PendingProxyGroups.resize(numThreads);

        staticMeshProxyPackage.PendingReplicationGroups.resize(numThreads);
        staticMeshProxyPackage.PendingProxyGroups.resize(numThreads);

        lightProxyPackage.PendingReplicationGroups.resize(numThreads);
        renderableProxyPackage.PendingReplicationGroups.resize(numThreads);
        renderableProxyPackage.PendingProxyGroups.resize(numThreads);
        renderableIndicesGroups.resize(numThreads);

        renderableIndices.reserve(kNumInitRenderableIndices);
        lightIndices.reserve(kNumInitLightIndices);
    }

    SceneProxy::~SceneProxy()
    {
        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            transformProxyPackage.Storage[localFrameIdx]->ForceReset();
            materialProxyPackage.Storage[localFrameIdx]->ForceReset();
            staticMeshProxyPackage.Storage[localFrameIdx]->ForceReset();
            renderableProxyPackage.Storage[localFrameIdx]->ForceReset();
            lightProxyPackage.Storage[localFrameIdx]->ForceReset();

            renderContext->DestroyGpuView(renderableIndicesBufferSrv[localFrameIdx]);
            renderContext->DestroyGpuView(lightIndicesBufferSrv[localFrameIdx]);

            renderContext->DestroyBuffer(renderableIndicesBuffer[localFrameIdx]);
            renderContext->DestroyBuffer(lightIndicesBuffer[localFrameIdx]);

            renderContext->DestroyBuffer(stagingBuffer[localFrameIdx]);
        }
    }

    GpuSyncPoint SceneProxy::Replicate(const LocalFrameIndex localFrameIdx, const World& world)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        ZoneScoped;

        const Registry& registry = world.GetRegistry();

        tf::Executor& executor = Engine::GetTaskExecutor();
        tf::Taskflow rootTaskFlow{};
        tf::Task updateTransformTask = rootTaskFlow.emplace(
            [this, localFrameIdx, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateTransformProxy");
                UpdateTransformProxy(subflow, localFrameIdx, registry);
                subflow.join();
            });

        tf::Task updateMaterialTask = rootTaskFlow.emplace(
            [this, localFrameIdx]()
            {
                // Material의 경우 분산 처리하기에는 상대적으로 그 수가 매우 작을 수 있기 때문에
                // 별도의 분산처리는 하지 않는다.
                ZoneScopedN("UpdateMaterialProxy");

                // 현재 스레드의 로그를 잠시 막을 수 있지만, 만약 work stealing이 일어난다면
                // 다른 부분에서 발생하는 로그가 기록되지 않는 현상이 일어 날 수도 있음.
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateMaterialProxy(localFrameIdx);
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            });

        tf::Task updateStaticMeshTask = rootTaskFlow.emplace(
            [this, localFrameIdx, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateStaticMeshProxy");
                UpdateStaticMeshProxy(subflow, localFrameIdx, registry);
                subflow.join();
            });

        tf::Task updateRenderableTask = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateRenderableTask");
                UpdateRenderableProxy(subflow, localFrameIdx);
                subflow.join();
            });

        updateStaticMeshTask.succeed(updateMaterialTask);
        updateRenderableTask.succeed(updateTransformTask, updateStaticMeshTask);

        //// replicate가 각 update 이후에 바로 이뤄지게 하기 위해서는 staging buffer의 독립이 1순위
        // tf::Task replicateTransformData = rootTaskFlow.emplace([]() {});
        // tf::Task replicateMaterialData = rootTaskFlow.emplace([]() {});
        // tf::Task replicateStaticMeshData = rootTaskFlow.emplace([]() {});
        // tf::Task replicateRenderableData = rootTaskFlow.emplace([]() {});
        // tf::Task replicateRenderableIndices = rootTaskFlow.emplace([]() {});

        // replicateTransformData.succeed(updateTransformTask);
        // replicateMaterialData.succeed(updateMaterialTask);
        // replicateStaticMeshData.succeed(updateStaticMeshTask);
        //// 이하 두개 태스크는 추후 렌더러블로 다뤄질 수 있는 종류가 많아 질수록 종속성을 추가해야함.
        // replicateRenderableData.succeed(updateRenderableTask);
        // replicateRenderableIndices.succeed(updateRenderableTask);

        executor.run(rootTaskFlow).wait();

        // #sy_warn Copy시 Alignment 문제 발생 가능!
        const Size transformReplicationSize = TransformProxy::kDataSize * transformProxyPackage.PendingReplications.size();
        const Size transformStagingOffset = 0;

        const Size materialReplicationSize = MaterialProxy::kDataSize * materialProxyPackage.PendingReplications.size();
        const Size materialStagingOffset = transformReplicationSize;

        const Size staticMeshReplicationSize = StaticMeshProxy::kDataSize * staticMeshProxyPackage.PendingReplications.size();
        const Size staticMeshStagingOffset = materialStagingOffset + materialReplicationSize;

        const Size renderableReplicationSize = RenderableProxy::kDataSize * renderableProxyPackage.PendingReplications.size();
        const Size renderableStagingOffset = staticMeshStagingOffset + staticMeshReplicationSize;

        const Size renderableIndicesSize = sizeof(U32) * renderableIndices.size();
        const Size renderableIndicesStagingOffset = renderableStagingOffset + renderableReplicationSize;

        const Size requiredStagingBufferSize = renderableIndicesStagingOffset + renderableIndicesSize;
        if (requiredStagingBufferSize == 0)
        {
            return GpuSyncPoint::Invalid();
        }

        PrepareStagingBuffer(localFrameIdx, requiredStagingBufferSize);
        IG_CHECK(stagingBuffer[localFrameIdx]);

        CommandListPool& asyncCopyCmdListPool = renderContext->GetAsyncCopyCommandListPool();

        auto transformCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "TransformProxyReplication"_fs);
        auto materialCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "MaterialProxyReplication"_fs);
        auto staticMeshCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "StaticMeshProxyReplication"_fs);
        auto renderableCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "RenderableProxyReplication"_fs);
        auto renderableIndicesCopyCmdList = asyncCopyCmdListPool.Request(localFrameIdx, "RenderableIndicesReplication"_fs);

        // 상대적으로 업로드 시간이 작을 가능성이 높은 데이터를 묶는다
        std::future<void> lightWeightRepFuture = std::async(
            std::launch::async,
            [this, localFrameIdx, &materialCopyCmdList, materialStagingOffset, &staticMeshCopyCmdList, staticMeshStagingOffset, &renderableCopyCmdList, renderableStagingOffset]
            {
                ZoneScopedN("LightWeightDataRep");
                ReplicatePrxoyData(localFrameIdx,
                                   materialProxyPackage,
                                   *materialCopyCmdList,
                                   materialStagingOffset);

                ReplicatePrxoyData(localFrameIdx,
                                   staticMeshProxyPackage,
                                   *staticMeshCopyCmdList,
                                   staticMeshStagingOffset);

                ReplicatePrxoyData(localFrameIdx,
                                   renderableProxyPackage,
                                   *renderableCopyCmdList,
                                   renderableStagingOffset);
            });

        // 현재 프레임에서 존재 할 수 있는 Renderable/Light의 수를 카운트(numCurrentRenderables/numCurrentLights) 해서
        // 만약 각 인덱스 버퍼의 최대 수용 가능 인덱스 수(numMaxRenderables/numMaxLights)를 초과하면, GpuBuffer의
        // 크기를 재조정 해야한다. (ex. max(numMaxRenderables, numCurrentRenderables * 2))
        std::future<void> renderableIndicesRepFuture = std::async(
            std::launch::async,
            [this, localFrameIdx, &renderableIndicesCopyCmdList, renderableIndicesStagingOffset, renderableIndicesSize]
            {
                ZoneScopedN("RenderableIndicesRep");
                ReplicateRenderableIndices(localFrameIdx,
                                           *renderableIndicesCopyCmdList,
                                           renderableIndicesStagingOffset, renderableIndicesSize);
            });

        {
            ZoneScopedN("TransformRep");
            ReplicatePrxoyData(localFrameIdx,
                               transformProxyPackage,
                               *transformCopyCmdList,
                               transformStagingOffset);
        }

        lightWeightRepFuture.get();
        renderableIndicesRepFuture.get();
        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        GpuSyncPoint syncPoint = asyncCopyQueue.MakeSyncPointWithSignal(renderContext->GetAsyncCopyFence());
        return syncPoint;
    }

    void SceneProxy::UpdateTransformProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        IG_CHECK(transformProxyPackage.PendingReplications.empty());
        IG_CHECK(transformProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];
        auto invalidateProxyTask = subflow.for_each(entityProxyMap.begin(), entityProxyMap.end(),
                                                    [](auto& keyValuePair)
                                                    {
                                                        keyValuePair.second.bMightBeDestroyed = true;
                                                    });

        auto& storage = *transformProxyPackage.Storage[localFrameIdx];
        const auto transformView = registry.view<const TransformComponent>();
        const auto numWorkGroups = (int)std::thread::hardware_concurrency();

        tf::Task createNewProxyTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &registry, &entityProxyMap, transformView, numWorkGroups](const int groupIdx)
            {
                ZoneScopedN("CreateNewPendingTransformProxyWorkGroup");
                const auto numWorksPerGroup = (int)transformView.size() / numWorkGroups;
                const auto numRemainedWorks = (int)transformView.size() % numWorkGroups;
                const Size numWorks = (Size)numWorksPerGroup + (groupIdx == 0 ? numRemainedWorks : 0);
                const Size viewOffset = (Size)(groupIdx * numWorksPerGroup) + (groupIdx != 0 ? numRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = *(transformView.begin() + viewOffset + viewIdx);
                    if (!entityProxyMap.contains(entity))
                    {
                        transformProxyPackage.PendingProxyGroups[groupIdx].emplace_back(entity, TransformProxy{});
                    }
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage, numWorkGroups]()
            {
                ZoneScopedN("CommitPendingTransformProxy");
                for (Index groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : transformProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    transformProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task updateProxyTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &registry, &entityProxyMap, transformView, numWorkGroups](const Size groupIdx)
            {
                ZoneScopedN("UpdateTransformWorkGruop");
                const auto numWorksPerGroup = (int)transformView.size() / numWorkGroups;
                const auto numRemainedWorks = (int)transformView.size() % numWorkGroups;
                const Size numWorks = (Size)numWorksPerGroup + (groupIdx == 0 ? numRemainedWorks : 0);
                const Size viewOffset = (groupIdx * numWorksPerGroup) + (groupIdx != 0 ? numRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = *(transformView.begin() + viewOffset + viewIdx);

                    TransformProxy& proxy = entityProxyMap[entity];
                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;

                    const TransformComponent& transformComponent = registry.get<TransformComponent>(entity);
                    if (const U64 currentDataHashValue = HashInstance(transformComponent);
                        proxy.DataHashValue != currentDataHashValue)
                    {
                        proxy.GpuData = transformComponent.CreateTransformation();
                        proxy.DataHashValue = currentDataHashValue;

                        transformProxyPackage.PendingReplicationGroups[groupIdx].emplace_back(entity);
                    }
                }
            });

        tf::Task commitReplications = subflow.emplace(
            [this, numWorkGroups]()
            {
                ZoneScopedN("CommitTransformReplications");
                for (int groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    for (const Entity entity : transformProxyPackage.PendingReplicationGroups[groupIdx])
                    {
                        transformProxyPackage.PendingReplications.emplace_back(entity);
                    }
                    transformProxyPackage.PendingReplicationGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage, localFrameIdx]()
            {
                ZoneScopedN("CommitTransformDestructions");
                for (auto& [entity, proxy] : entityProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        transformProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : transformProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = transformProxyPackage.ProxyMap[localFrameIdx].extract(entity);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                transformProxyPackage.PendingDestructions.clear();
            });

        invalidateProxyTask.precede(createNewProxyTask);
        createNewProxyTask.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(updateProxyTask);
        updateProxyTask.precede(commitReplications, commitDestructions);
    }

    void SceneProxy::UpdateMaterialProxy(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(materialProxyPackage.PendingReplications.empty());
        IG_CHECK(materialProxyPackage.PendingDestructions.empty());

        auto& proxyMap = materialProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *materialProxyPackage.Storage[localFrameIdx];

        for (auto& [material, proxy] : proxyMap)
        {
            IG_CHECK(material);
            proxy.bMightBeDestroyed = true;
        }

        std::vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::Material)};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::Material);

            if (!snapshot.IsCached())
            {
                continue;
            }

            ManagedAsset<Material> cachedMaterial = assetManager->LoadMaterial(snapshot.Info.GetGuid(), true);
            IG_CHECK(cachedMaterial);

            if (!proxyMap.contains(cachedMaterial))
            {
                proxyMap[cachedMaterial] = MaterialProxy{.StorageSpace = storage.Allocate(1)};
            }

            MaterialProxy& proxy = proxyMap[cachedMaterial];
            IG_CHECK(proxy.bMightBeDestroyed);
            proxy.bMightBeDestroyed = false;

            const Material* materialPtr = assetManager->Lookup(cachedMaterial);
            IG_CHECK(materialPtr != nullptr);

            MaterialGpuData newData{};
            const Texture* diffuseTexturePtr = assetManager->Lookup(materialPtr->GetDiffuse());
            if (diffuseTexturePtr != nullptr)
            {
                const GpuView* srvPtr = renderContext->Lookup(diffuseTexturePtr->GetShaderResourceView());
                IG_CHECK(srvPtr != nullptr);
                newData.DiffuseTextureSrv = srvPtr->Index;

                const GpuView* sampler = renderContext->Lookup(diffuseTexturePtr->GetSampler());
                IG_CHECK(sampler != nullptr);
                newData.DiffuseTextureSampler = sampler->Index;
            }

            if (const U64 currentDataHashValue = HashInstance(newData);
                proxy.DataHashValue != currentDataHashValue)
            {
                proxy.GpuData = newData;
                proxy.DataHashValue = currentDataHashValue;
                materialProxyPackage.PendingReplications.emplace_back(cachedMaterial);
            }

            assetManager->Unload(cachedMaterial, true);
        }

        for (auto& [material, proxy] : proxyMap)
        {
            if (proxy.bMightBeDestroyed)
            {
                materialProxyPackage.PendingDestructions.emplace_back(material);
            }
        }

        for (const ManagedAsset<Material> material : materialProxyPackage.PendingDestructions)
        {
            const auto extractedElement = materialProxyPackage.ProxyMap[localFrameIdx].extract(material);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        materialProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateStaticMeshProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        IG_CHECK(staticMeshProxyPackage.PendingReplications.empty());
        IG_CHECK(staticMeshProxyPackage.PendingDestructions.empty());

        const auto& materialProxyMap = materialProxyPackage.ProxyMap[localFrameIdx];

        auto& entityProxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        tf::Task invalidateProxyTask = subflow.for_each(entityProxyMap.begin(), entityProxyMap.end(),
                                                        [](auto& keyValuePair)
                                                        {
                                                            keyValuePair.second.bMightBeDestroyed = true;
                                                        });

        auto& storage = *staticMeshProxyPackage.Storage[localFrameIdx];
        const auto staticMeshEntityView = registry.view<const StaticMeshComponent>();
        const auto numWorkGroups = (int)std::thread::hardware_concurrency();

        tf::Task createNewProxyTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &registry, &entityProxyMap, staticMeshEntityView, numWorkGroups](const int groupIdx)
            {
                ZoneScopedN("CreateNewPendingStaticMeshProxy");
                const auto numWorksPerGroup = (int)staticMeshEntityView.size() / numWorkGroups;
                const auto numRemainedWorks = (int)staticMeshEntityView.size() % numWorkGroups;
                const Size numWorks = (Size)numWorksPerGroup + (groupIdx == 0 ? numRemainedWorks : 0);
                const Size viewOffset = (Size)(groupIdx * numWorksPerGroup) + (groupIdx != 0 ? numRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = *(staticMeshEntityView.begin() + viewOffset + viewIdx);
                    if (!entityProxyMap.contains(entity))
                    {
                        const StaticMeshComponent& staticMeshComponent = staticMeshEntityView.get<const StaticMeshComponent>(entity);
                        if (!staticMeshComponent.Mesh)
                        {
                            continue;
                        }

                        staticMeshProxyPackage.PendingProxyGroups[groupIdx].emplace_back(entity, StaticMeshProxy{});
                    }
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage, numWorkGroups]()
            {
                ZoneScopedN("CommitPendingStaticMeshProxy");
                for (Index groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : staticMeshProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    staticMeshProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task updateProxyTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &registry, &entityProxyMap, &materialProxyMap, staticMeshEntityView, numWorkGroups](const Size groupIdx)
            {
                ZoneScopedN("UpdateStaticMeshPerWorkGruop");
                const auto numWorksPerGroup = (int)staticMeshEntityView.size() / numWorkGroups;
                const auto numRemainedWorks = (int)staticMeshEntityView.size() % numWorkGroups;

                // 에셋 매니저에서 미리 material 처럼 가져온 다음에
                // static mesh handle - static mesh ptr 맵을 만들고 참조하기 (read만하기)
                std::vector<AssetManager::Snapshot> staticMeshSnapshots{assetManager->TakeSnapshots(EAssetCategory::StaticMesh)};
                UnorderedMap<ManagedAsset<StaticMesh>, const StaticMesh*> staticMeshTable{};
                for (const AssetManager::Snapshot& staticMeshSnapshot : staticMeshSnapshots)
                {
                    if (staticMeshSnapshot.IsCached())
                    {
                        // 조금 꼼수긴 하지만, 여전히 valid 한 방식이라고 생각한다.
                        // 잘못된 핸들은 애초에 Lookup이 불가능하고. 애초에 핸들 자체가 소유권을 가질 핸들 만 제대로
                        // 이해하고 해제 해준다면 문제 없기 때문.
                        // 성능면에선 실제로 렌더링 될 메시보다 유효한 에셋의 수가 더 적을 것이라고 예측되고
                        // Lookup에 의한 mutex-lock의 횟수 또한 현저히 적거나 거의 없을 것이라고 생각된다.
                        const ManagedAsset<StaticMesh> reconstructedHandle{staticMeshSnapshot.HandleHash};
                        staticMeshTable[reconstructedHandle] = assetManager->Lookup(reconstructedHandle);
                    }
                }

                const Size numWorks = (Size)numWorksPerGroup + (groupIdx == 0 ? numRemainedWorks : 0);
                const Size viewOffset = (groupIdx * numWorksPerGroup) + (groupIdx != 0 ? numRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = *(staticMeshEntityView.begin() + viewOffset + viewIdx);

                    StaticMeshProxy& proxy = entityProxyMap[entity];
                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;

                    const StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
                    const StaticMesh* staticMeshPtr = staticMeshTable[staticMeshComponent.Mesh];
                    IG_CHECK(staticMeshPtr != nullptr);

                    // Mesh Storage에서 각 핸들은 고유하고, 해제 되기 전까지 해당 핸들에 대한 내부 값은 절대 바뀌지 않는다.
                    const MeshStorage::Handle<U32> vertexIndexSpace = staticMeshPtr->GetVertexIndexSpace();
                    const ManagedAsset<Material> material = staticMeshPtr->GetMaterial();
                    IG_CHECK(materialProxyMap.contains(material));
                    if (const U64 currentDataHashValue = vertexIndexSpace.Value ^ material.Value;
                        proxy.DataHashValue != currentDataHashValue)
                    {
                        const MeshStorage::Handle<VertexSM> vertexSpace = staticMeshPtr->GetVertexSpace();
                        const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage->Lookup(vertexSpace);
                        const MeshStorage::Space<U32>* vertexIndexSpacePtr = meshStorage->Lookup(vertexIndexSpace);

                        proxy.GpuData = StaticMeshGpuData{
                            .MaterialDataIdx = (U32)materialProxyMap.at(material).StorageSpace.OffsetIndex,
                            .VertexOffset = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.OffsetIndex : 0),
                            .NumVertices = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.NumElements : 0),
                            .IndexOffset = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.OffsetIndex : 0),
                            .NumIndices = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.NumElements : 0)};
                        proxy.DataHashValue = currentDataHashValue;

                        staticMeshProxyPackage.PendingReplicationGroups[groupIdx].emplace_back(entity);
                    }
                }
            });

        tf::Task commitReplications = subflow.emplace(
            [this, numWorkGroups]()
            {
                ZoneScopedN("CommitStaticMeshReplication");
                for (int groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    for (const Entity entity : staticMeshProxyPackage.PendingReplicationGroups[groupIdx])
                    {
                        staticMeshProxyPackage.PendingReplications.emplace_back(entity);
                    }
                    staticMeshProxyPackage.PendingReplicationGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage, localFrameIdx]()
            {
                ZoneScopedN("CommitStaticMeshDestructions");
                for (auto& [entity, proxy] : entityProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        staticMeshProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : staticMeshProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = entityProxyMap.extract(entity);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                staticMeshProxyPackage.PendingDestructions.clear();
            });

        invalidateProxyTask.precede(createNewProxyTask);
        createNewProxyTask.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(updateProxyTask);
        updateProxyTask.precede(commitReplications, commitDestructions);
    }

    void SceneProxy::UpdateRenderableProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(renderableProxyPackage.PendingReplications.empty());
        IG_CHECK(renderableProxyPackage.PendingDestructions.empty());

        auto& renderableProxyMap = renderableProxyPackage.ProxyMap[localFrameIdx];
        auto& renderableStorage = *renderableProxyPackage.Storage[localFrameIdx];
        const auto& staticMeshProxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        const auto& transformProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];

        tf::Task invalidateProxyTask = subflow.for_each(renderableProxyMap.begin(), renderableProxyMap.end(),
                                                        [](auto& keyValuePair)
                                                        {
                                                            keyValuePair.second.bMightBeDestroyed = true;
                                                        });

        // 만약 Renderable Component를 한 개의 Entity가 여러개 가지고 있더라도
        // renderable로 반영 되는 건 딱 첫 번째로 인식 되는 컴포넌트다.
        // TODO 이런 경우 처리를 따로 해야할까?
        // 가능한 방안은 같은 타입의 Renderable이 아니라면 bMightBeDestroyed를 false로 변경하지 않도록 한다.
        // 그렇게 하면 가장 먼저 업로드된 정보가 Expired 되어도 정상적으로 해당 Renderable에 대한 Destroy 알고리즘이
        // 작동하게 되고, 다음 프레임 부터는 정상적으로 새로운 Renderable이 할당되게 된다.
        const auto numWorkGroups = (int)std::thread::hardware_concurrency();

        // 확장시 2가지 선택 가능한 선택지.
        // 1. 한번에 하나의 타입에 대해서 순차적 처리 => 이 방식을 선제 구현 후 프로파일링 후 추가 조치 할 것!
        // 2. 각각 개별적인 pending list를 가져서 전부 개별적으로 동시적으로 처리
        // (ex. pendingStaticMeshRenderableProxy, pendingSkeletalMeshRenderableProxy, ...etc)
        // Static Mesh
        tf::Task createRenderableStaticMeshTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &renderableProxyMap, &staticMeshProxyMap, &transformProxyMap, numWorkGroups](const int groupIdx)
            {
                const auto numStaticMeshWorksPerGroup = (int)staticMeshProxyMap.size() / numWorkGroups;
                const auto numStaticMeshRemainedWorks = (int)staticMeshProxyMap.size() % numWorkGroups;
                const Size numWorks = (Size)numStaticMeshWorksPerGroup + (groupIdx == 0 ? numStaticMeshRemainedWorks : 0);
                const Size viewOffset = (Size)(groupIdx * numStaticMeshWorksPerGroup) + (groupIdx != 0 ? numStaticMeshRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = (staticMeshProxyMap.cbegin() + viewOffset + viewIdx)->first;
                    IG_CHECK(transformProxyMap.contains(entity));
                    if (!renderableProxyMap.contains(entity))
                    {
                        renderableProxyPackage.PendingProxyGroups[groupIdx].emplace_back(
                            entity,
                            RenderableProxy{
                                .StorageSpace = {},
                                .GpuData = RenderableGpuData{
                                    .Type = ERenderableType::StaticMesh,
                                    .DataIdx = (U32)staticMeshProxyMap.at(entity).StorageSpace.OffsetIndex,
                                    .TransformDataIdx = (U32)transformProxyMap.at(entity).StorageSpace.OffsetIndex}});

                        renderableProxyPackage.PendingReplicationGroups[groupIdx].emplace_back(entity);
                    }
                }
            });

        // 모든 종류의 Renderable에 대한 Proxy가 만들어 진 이후
        tf::Task commitPendingRenderableTask = subflow.emplace(
            [this, numWorkGroups, &renderableProxyMap, &renderableStorage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    // Commit Pending Renderables
                    for (auto& pendingRenderable : renderableProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingRenderable.second.StorageSpace = renderableStorage.Allocate(1);
                        renderableProxyMap[pendingRenderable.first] = pendingRenderable.second;
                    }
                    renderableProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        // Storage 할당이 끝나면 각 종류의 Renderable들에 대한 proxy 유효성(수명) 평가와 renderable indices 구성
        tf::Task constructStaticMeshRenderableIndicesTask = subflow.for_each_index(
            0, numWorkGroups, 1,
            [this, &renderableProxyMap, &staticMeshProxyMap, numWorkGroups](const int groupIdx)
            {
                const auto numStaticMeshWorksPerGroup = (int)staticMeshProxyMap.size() / numWorkGroups;
                const auto numStaticMeshRemainedWorks = (int)staticMeshProxyMap.size() % numWorkGroups;
                const Size numWorks = (Size)numStaticMeshWorksPerGroup + (groupIdx == 0 ? numStaticMeshRemainedWorks : 0);
                const Size viewOffset = (Size)(groupIdx * numStaticMeshWorksPerGroup) + (groupIdx != 0 ? numStaticMeshRemainedWorks : 0);
                for (Size viewIdx = 0; viewIdx < numWorks; ++viewIdx)
                {
                    const Entity entity = (staticMeshProxyMap.begin() + viewOffset + viewIdx)->first;
                    RenderableProxy& proxy = renderableProxyMap[entity];
                    if (proxy.GpuData.Type != ERenderableType::StaticMesh)
                    {
                        continue;
                    }

                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;
                    renderableIndicesGroups[groupIdx].emplace_back((U32)proxy.StorageSpace.OffsetIndex);
                }
            });

        tf::Task commitPendingRenderableIndicesTask = subflow.emplace(
            [this, numWorkGroups]()
            {
                renderableIndices.clear();
                for (Index groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    for (const U32 renderableIdx : renderableIndicesGroups[groupIdx])
                    {
                        renderableIndices.emplace_back(renderableIdx);
                    }
                    renderableIndicesGroups[groupIdx].clear();
                }
            });

        tf::Task commitPendingReplicationsTask = subflow.emplace(
            [this, numWorkGroups, &renderableProxyMap, &renderableStorage]()
            {
                renderableProxyPackage.PendingReplications.clear();
                for (Index groupIdx = 0; groupIdx < numWorkGroups; ++groupIdx)
                {
                    // Commit Replications
                    for (const Entity pendingReplicationEntity : renderableProxyPackage.PendingReplicationGroups[groupIdx])
                    {
                        renderableProxyPackage.PendingReplications.emplace_back(pendingReplicationEntity);
                    }
                    renderableProxyPackage.PendingReplicationGroups[groupIdx].clear();
                }
            });

        tf::Task commitPendingDestructionsTask = subflow.emplace(
            [this, &renderableProxyMap, &renderableStorage]()
            {
                for (auto& [entity, proxy] : renderableProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        renderableProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : renderableProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = renderableProxyMap.extract(entity);
                    renderableStorage.Deallocate(extractedElement->second.StorageSpace);
                }
                renderableProxyPackage.PendingDestructions.clear();
            });

        invalidateProxyTask.precede(createRenderableStaticMeshTask);
        createRenderableStaticMeshTask.precede(commitPendingRenderableTask);
        commitPendingRenderableTask.precede(constructStaticMeshRenderableIndicesTask);
        constructStaticMeshRenderableIndicesTask.precede(commitPendingRenderableIndicesTask,
                                                         commitPendingReplicationsTask,
                                                         commitPendingDestructionsTask);
    }

    void SceneProxy::PrepareStagingBuffer(const LocalFrameIndex localFrameIdx, const Size requiredSize)
    {
        ZoneScoped;
        if (stagingBufferSize[localFrameIdx] < requiredSize)
        {
            if (stagingBuffer[localFrameIdx])
            {
                GpuBuffer* oldStagingBufferPtr = renderContext->Lookup(stagingBuffer[localFrameIdx]);
                oldStagingBufferPtr->Unmap();
                mappedStagingBuffer[localFrameIdx] = nullptr;
                renderContext->DestroyBuffer(stagingBuffer[localFrameIdx]);
            }

            const Size newStagingBufferSize = std::max(stagingBufferSize[localFrameIdx] * 2, requiredSize);
            GpuBufferDesc stagingBufferDesc{};
            stagingBufferDesc.AsUploadBuffer((U32)newStagingBufferSize);
            stagingBufferDesc.DebugName = String(std::format("SceneProxyStagingBuffer.{}", localFrameIdx));
            stagingBuffer[localFrameIdx] = renderContext->CreateBuffer(stagingBufferDesc);
            stagingBufferSize[localFrameIdx] = newStagingBufferSize;

            GpuBuffer* stagingBufferPtr = renderContext->Lookup(stagingBuffer[localFrameIdx]);
            mappedStagingBuffer[localFrameIdx] = stagingBufferPtr->Map(0);
        }
    }

    template <typename Proxy, typename Owner>
    void SceneProxy::ReplicatePrxoyData(const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage,
                                        CommandList& cmdList,
                                        const Size stagingBufferOffset)
    {
        // <1>. UploadInfo = GpuStorageAllocation, GpuData에 대한 참조를 수집
        //
        // <2>. GpuStorageAllocation의 OffsetIndex에 따라 Uploadinfo 정렬
        //
        // <3>. GpuUploader로 부터 UploadContext 요청 (확보해야할 크기 = proxy::kDataSize * numPendingReplications)
        //
        // <4>. GpuUploader로 부터 할당받은 (mapping된) Upload Buffer 공간에 순서대로 데이터를 써넣는다
        //
        // <5>. GpuStorage 공간에서의 연속성에 따라 Copy 명령어를 Batching 한다.
        // (srcOffset = 0, copySize = 0, dstOffset = UploadInfos[0].StorageSpaceRef.Offset)
        //
        // <5-1>. 조건 (UploadInfos[currentIdx].StorageSpaceRef.OffsetIndex + 1) == UploadInfos[currentIdx + 1].StorageSpaceRef.OffsetIndex
        // 이 만족하면 Storage 공간 상에서 물리적으로 연속적이다. 그러므로 copySize 만 증가시키고 currentIdx를 증가시킨다.
        //
        // <5-2>. 위 조건이 만족하지 않거나 (currentIdx + 1) >= numPendingReplications 라면
        // CopyBuffer(srcOffset, copySize, storageBuffer, dstOffset)
        // srcOffset += copySize, copySize = 0, dstOffset = dstOffset = UploadInfos[currentIdx + 1].StorageSpaceRef.Offset

        // <1>.
        if (!proxyPackage.PendingReplications.empty())
        {
            Vector<typename Proxy::UploadInfo> uploadInfos;
            uploadInfos.reserve(proxyPackage.PendingReplications.size());
            for (const Owner owner : proxyPackage.PendingReplications)
            {
                IG_CHECK(proxyPackage.ProxyMap[localFrameIdx].contains(owner));

                const Proxy& proxy = proxyPackage.ProxyMap[localFrameIdx][owner];
                uploadInfos.emplace_back(
                    typename Proxy::UploadInfo{
                        .StorageSpaceRef = CRef{proxy.StorageSpace},
                        .GpuDataRef = CRef{proxy.GpuData}});
            }
            IG_CHECK(!uploadInfos.empty());

            // <2>.
            std::sort(uploadInfos.begin(), uploadInfos.end(),
                      [](const Proxy::UploadInfo& lhs, const Proxy::UploadInfo& rhs)
                      {
                          return lhs.StorageSpaceRef.get().Offset < rhs.StorageSpaceRef.get().Offset;
                      });

            // <3>.
            GpuBuffer* stagingBufferPtr = renderContext->Lookup(stagingBuffer[localFrameIdx]);
            IG_CHECK(stagingBufferPtr != nullptr);

            // <4 ~ 5>.
            const RenderHandle<GpuBuffer> storageBuffer = proxyPackage.Storage[localFrameIdx]->GetGpuBuffer();
            GpuBuffer* storageBufferPtr = renderContext->Lookup(storageBuffer);
            IG_CHECK(storageBufferPtr != nullptr);

            IG_CHECK(mappedStagingBuffer[localFrameIdx] != nullptr);
            auto* mappedUploadBuffer = reinterpret_cast<Proxy::GpuData_t*>(mappedStagingBuffer[localFrameIdx] + stagingBufferOffset);
            Size srcOffset = stagingBufferOffset;
            Size copySize = 0;
            Size dstOffset = uploadInfos[0].StorageSpaceRef.get().Offset;

            cmdList.Open();
            for (Size idx = 0; idx < uploadInfos.size(); ++idx)
            {
                copySize += Proxy::kDataSize;
                mappedUploadBuffer[idx] = uploadInfos[idx].GpuDataRef.get();

                const bool bHasNextUploadInfo = (idx + 1) < uploadInfos.size();
                // 현재 업로드 정보가 마지막이거나, 다음 업로드 정보가 Storage에서 불연속적일 때.
                const bool bPendingCopyCommand = !bHasNextUploadInfo ||
                    ((uploadInfos[idx].StorageSpaceRef.get().OffsetIndex + 1) != uploadInfos[idx + 1].StorageSpaceRef.get().OffsetIndex);
                if (bPendingCopyCommand)
                {
                    cmdList.CopyBuffer(*stagingBufferPtr, srcOffset, copySize, *storageBufferPtr, dstOffset);
                }

                const bool bPendingNextBatching = bHasNextUploadInfo && bPendingCopyCommand;
                if (bPendingNextBatching)
                {
                    IG_CHECK((uploadInfos[idx].StorageSpaceRef.get().OffsetIndex + 1) <= uploadInfos[idx + 1].StorageSpaceRef.get().OffsetIndex);
                    srcOffset += copySize;
                    copySize = 0;
                    dstOffset = uploadInfos[idx + 1].StorageSpaceRef.get().Offset;
                }
            }
            cmdList.Close();

            CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
            CommandList* cmdLists[]{&cmdList};
            asyncCopyQueue.ExecuteContexts(cmdLists);

            proxyPackage.PendingReplications.clear();
        }
    }

    void SceneProxy::ReplicateRenderableIndices(const LocalFrameIndex localFrameIdx,
                                                CommandList& cmdList,
                                                const Size stagingBufferOffset, const Size replicationSize)
    {
        numMaxRenderables[localFrameIdx] = (U32)renderableIndices.size();

        if (numMaxRenderables[localFrameIdx] == 0)
        {
            return;
        }

        if (renderableIndicesBufferSize[localFrameIdx] < numMaxRenderables[localFrameIdx])
        {
            renderContext->DestroyGpuView(renderableIndicesBufferSrv[localFrameIdx]);
            renderContext->DestroyBuffer(renderableIndicesBuffer[localFrameIdx]);

            const U32 newBufferSize = std::max(numMaxRenderables[localFrameIdx], renderableIndicesBufferSize[localFrameIdx] * 2);
            GpuBufferDesc renderableIndicesBufferDesc;
            renderableIndicesBufferDesc.AsStructuredBuffer<U32>(newBufferSize);
            renderableIndicesBufferDesc.DebugName = String(std::format("RenderableIndicesBuffer{}", localFrameIdx));
            renderableIndicesBuffer[localFrameIdx] = renderContext->CreateBuffer(renderableIndicesBufferDesc);
            renderableIndicesBufferSrv[localFrameIdx] = renderContext->CreateShaderResourceView(renderableIndicesBuffer[localFrameIdx]);

            renderableIndicesBufferSize[localFrameIdx] = newBufferSize;
        }

        GpuBuffer* renderableIndicesBufferPtr = renderContext->Lookup(renderableIndicesBuffer[localFrameIdx]);
        IG_CHECK(renderableIndicesBufferPtr != nullptr);

        GpuBuffer* stagingBufferPtr = renderContext->Lookup(stagingBuffer[localFrameIdx]);
        IG_CHECK(stagingBufferPtr != nullptr);

        IG_CHECK(mappedStagingBuffer[localFrameIdx] != nullptr);
        auto* mappedBuffer = reinterpret_cast<U32*>(mappedStagingBuffer[localFrameIdx] + stagingBufferOffset);

        IG_CHECK(replicationSize == (sizeof(U32) * numMaxRenderables[localFrameIdx]));
        std::memcpy(mappedBuffer, renderableIndices.data(), replicationSize);

        cmdList.Open();
        cmdList.CopyBuffer(*stagingBufferPtr, stagingBufferOffset, replicationSize, *renderableIndicesBufferPtr, 0);
        cmdList.Close();

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        CommandList* cmdLists[]{&cmdList};
        asyncCopyQueue.ExecuteContexts(cmdLists);
    }
} // namespace ig
