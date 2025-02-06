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
#include "Igniter/Component/MaterialComponent.h"
#include "Igniter/Component/LightComponent.h"
#include "Igniter/Component/RenderableTag.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Render/SceneProxy.h"

namespace ig
{
    SceneProxy::SceneProxy(tf::Executor& taskExecutor, RenderContext& renderContext, const MeshStorage& meshStorage, AssetManager& assetManager)
        : taskExecutor(&taskExecutor)
        , renderContext(&renderContext)
        , meshStorage(&meshStorage)
        , assetManager(&assetManager)
        , numWorker((U32)taskExecutor.num_workers())
    {
        GpuBufferDesc renderableIndicesBufferDesc;
        renderableIndicesBufferDesc.AsStructuredBuffer<U32>(kNumInitRenderableIndices);

        transformProxyPackage.Storage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("TransformStorage")),
                    TransformProxy::kDataSize,
                    kNumInitTransformElements});

        materialProxyPackage.Storage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("MaterialDataStorage")),
                    MaterialProxy::kDataSize,
                    kNumInitMaterialElements});

        meshProxyPackage.Storage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("MeshDataStorage")),
                    MeshProxy::kDataSize,
                    kNumInitMeshElements});

        instancingPackage.InstancingDataStorage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("InstancingDataStorage")),
                    (U32)sizeof(InstancingGpuData),
                    kNumInitStaticMeshInstances,
                    EGpuStorageFlags::ShaderReadWrite | EGpuStorageFlags::EnableLinearAllocation});

        instancingPackage.IndirectTransformStorage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("IndirectTransformStorage")),
                    (U32)sizeof(U32),
                    kNumInitStaticMeshInstances,
                    EGpuStorageFlags::ShaderReadWrite | EGpuStorageFlags::EnableLinearAllocation});

        renderableProxyPackage.Storage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("RenderableStorage")),
                    RenderableProxy::kDataSize,
                    kNumInitRenderableElements});

        lightProxyPackage.Storage =
            MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    String(std::format("LightDataStorage")),
                    LightProxy::kDataSize,
                    kNumInitLightElements});

        renderableIndicesBufferSize = kNumInitRenderableIndices;
        renderableIndicesBufferDesc.DebugName = String(std::format("RenderableIndicesBuffer"));
        renderableIndicesBuffer = renderContext.CreateBuffer(renderableIndicesBufferDesc);
        renderableIndicesBufferSrv = renderContext.CreateShaderResourceView(renderableIndicesBuffer);

        meshProxyPackage.PendingReplicationGroups.resize(numWorker);
        meshProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        meshProxyPackage.WorkGroupCmdLists.resize(numWorker);

        materialProxyPackage.PendingReplicationGroups.resize(numWorker);
        materialProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        materialProxyPackage.WorkGroupCmdLists.resize(numWorker);

        transformProxyPackage.PendingReplicationGroups.resize(numWorker);
        transformProxyPackage.PendingProxyGroups.resize(numWorker);
        transformProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        transformProxyPackage.WorkGroupCmdLists.resize(numWorker);

        lightProxyPackage.PendingReplicationGroups.resize(numWorker);
        lightProxyPackage.PendingProxyGroups.resize(numWorker);
        lightProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        lightProxyPackage.WorkGroupCmdLists.resize(numWorker);

        instancingPackage.ThreadLocalInstancingMaps.resize(numWorker);
        localIntermediateStaticMeshData.resize(numWorker);

        lightProxyPackage.PendingReplicationGroups.resize(numWorker);

        renderableProxyPackage.PendingReplicationGroups.resize(numWorker);
        renderableProxyPackage.PendingProxyGroups.resize(numWorker);
        renderableProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        renderableProxyPackage.WorkGroupCmdLists.resize(numWorker);

        renderableIndicesGroups.resize(numWorker);
        renderableIndices.reserve(kNumInitRenderableIndices);
    }

    SceneProxy::~SceneProxy()
    {
        transformProxyPackage.Storage->ForceReset();
        materialProxyPackage.Storage->ForceReset();
        meshProxyPackage.Storage->ForceReset();
        renderableProxyPackage.Storage->ForceReset();
        lightProxyPackage.Storage->ForceReset();

        renderContext->DestroyGpuView(renderableIndicesBufferSrv);
        renderContext->DestroyBuffer(renderableIndicesBuffer);

        for (auto localFrameIdx : LocalFramesView)
        {
            renderContext->DestroyBuffer(renderableIndicesStagingBuffer[localFrameIdx]);
        }
    }

    GpuSyncPoint SceneProxy::Replicate(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint& prevFrameSyncPoint)
    {
        IG_CHECK(taskExecutor != nullptr);
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        ZoneScoped;

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        asyncCopyQueue.Wait(prevFrameSyncPoint);
        IG_CHECK(asyncCopyQueue.GetType() == EQueueType::Copy);

        if (invalidationFuture[localFrameIdx].valid())
        {
            invalidationFuture[localFrameIdx].wait();
        }

        const Registry& registry = world.GetRegistry();

        tf::Taskflow rootTaskFlow{};
        tf::Task updateTransformTask = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateTransformProxy");
                UpdateTransformProxy(subflow, registry);
                subflow.join();
            });

        tf::Task updateLightTask = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateLightProxy");
                UpdateLightProxy(subflow, registry);
                subflow.join();
            });

        tf::Task updateMaterialTask = rootTaskFlow.emplace(
            [this]()
            {
                // Material의 경우 분산 처리하기에는 상대적으로 그 수가 매우 작을 수 있기 때문에
                // 별도의 분산처리는 하지 않는다.
                ZoneScopedN("UpdateMaterialProxy");

                // 현재 스레드의 로그를 잠시 막을 수 있지만, 만약 work stealing이 일어난다면
                // 다른 부분에서 발생하는 로그가 기록되지 않는 현상이 일어 날 수도 있음.
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateMaterialProxy();
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            });

        tf::Task updateMeshTask = rootTaskFlow.emplace(
            [this]()
            {
                ZoneScopedN("UpdateMeshProxy");
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateMeshProxy();
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            });

        tf::Task buildInstancingData = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("BuildInstancingData");
                BuildInstancingData(subflow, registry);
                subflow.join();
            });

        tf::Task updateRenderableTask = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateRenderableProxy");
                UpdateRenderableProxy(subflow);
                subflow.join();
            });

        buildInstancingData.succeed(updateMaterialTask, updateMeshTask);
        updateRenderableTask.succeed(updateTransformTask, buildInstancingData);

        tf::Task replicateTransformData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateTransformData");
                ReplicateProxyData(subflow, localFrameIdx, transformProxyPackage);
                subflow.join();
            });

        tf::Task replicateLightData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateLightData");
                ReplicateProxyData(subflow, localFrameIdx, lightProxyPackage);
                subflow.join();
            });

        tf::Task replicateMaterialData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateMaterialData");
                ReplicateProxyData(subflow, localFrameIdx, materialProxyPackage);
                subflow.join();
            });

        tf::Task replicateMeshData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateMeshData");
                ReplicateProxyData(subflow, localFrameIdx, meshProxyPackage);
                subflow.join();
            });

        tf::Task replicateInstancingData = rootTaskFlow.emplace(
            [this, localFrameIdx]()
            {
                ZoneScopedN("ReplicateInstancingData");
                ReplicateInstancingData(localFrameIdx);
            });

        tf::Task replicateRenderableData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateRenderableData");
                ReplicateProxyData(subflow, localFrameIdx, renderableProxyPackage);
                subflow.join();
            });

        tf::Task replicateRenderableIndices = rootTaskFlow.emplace(
            [this, localFrameIdx]
            {
                ZoneScopedN("RenderableIndicesRep");
                ReplicateRenderableIndices(localFrameIdx);
            });

        replicateTransformData.succeed(updateTransformTask);
        replicateLightData.succeed(updateLightTask);
        replicateMaterialData.succeed(updateMaterialTask);
        replicateMeshData.succeed(updateMeshTask);
        replicateInstancingData.succeed(buildInstancingData);
        //// 이하 두개 태스크는 추후 렌더러블로 다뤄질 수 있는 종류가 많아 질수록 종속성을 추가해야함.
        replicateRenderableData.succeed(updateRenderableTask);
        replicateRenderableIndices.succeed(updateRenderableTask);

        // 현재 프레임 작업 시작
        taskExecutor->run(rootTaskFlow).wait();
        // 현재 프레임 작업 완료

        GpuSyncPoint syncPoint = asyncCopyQueue.MakeSyncPointWithSignal(renderContext->GetAsyncCopyFence());
        return syncPoint;
    }

    void SceneProxy::PrepareNextFrame(const LocalFrameIndex localFrameIdx)
    {
        // 다음 프레임 작업 준비
        const LocalFrameIndex nextLocalFrameIdx = (localFrameIdx + 1) % NumFramesInFlight;
        tf::Taskflow prepareNextFrameFlow{};
        tf::Task invalidateTransformProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameTransformProxy");
                subflow.for_each(
                    transformProxyPackage.ProxyMap.begin(), transformProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        tf::Task invalidateLightProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameLightProxy");
                subflow.for_each(
                    lightProxyPackage.ProxyMap.begin(), lightProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        tf::Task invalidateMeshProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx]()
            {
                ZoneScopedN("InvalidateNextFrameMeshProxy");
                for (auto& [staticMesh, proxy] : meshProxyPackage.ProxyMap)
                {
                    IG_CHECK(staticMesh);
                    proxy.bMightBeDestroyed = true;
                }
            });

        tf::Task invalidateMaterialProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx]()
            {
                ZoneScopedN("InvalidateNextFrameMaterialProxy");
                for (auto& [material, proxy] : materialProxyPackage.ProxyMap)
                {
                    IG_CHECK(material);
                    proxy.bMightBeDestroyed = true;
                }
            });

        tf::Task invalidateRenderableProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameRenderableProxy");
                subflow.for_each(
                    renderableProxyPackage.ProxyMap.begin(), renderableProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        tf::Task partialInvalidateInstancingData = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx]()
            {
                ZoneScopedN("PartiallyInvalidateNextFrameInstancingData");
                instancingPackage.InstancingDataStorage->ForceReset();
                instancingPackage.IndirectTransformStorage->ForceReset();
                for (InstancingMap& threadLocalMap : instancingPackage.ThreadLocalInstancingMaps)
                {
                    threadLocalMap.clear();
                }
            });

        invalidationFuture[nextLocalFrameIdx] = taskExecutor->run(std::move(prepareNextFrameFlow));
    }

    void SceneProxy::UpdateTransformProxy(tf::Subflow& subflow, const Registry& registry)
    {
        IG_CHECK(transformProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = transformProxyPackage.ProxyMap;
        auto& storage = *transformProxyPackage.Storage;
        const auto transformView = registry.view<const TransformComponent, RenderableTag>();

        tf::Task updateTransformProxy = subflow.for_each(
            transformView.begin(), transformView.end(),
            [this, &registry, &entityProxyMap, transformView](const Entity entity)
            {
                const Index workerId = taskExecutor->this_worker_id();
                const auto transformFoundItr = entityProxyMap.find(entity);
                if (transformFoundItr == entityProxyMap.end())
                {
                    transformProxyPackage.PendingProxyGroups[workerId].emplace_back(entity, TransformProxy{});
                    transformProxyPackage.PendingReplicationGroups[workerId].emplace_back(entity);
                }
                else
                {
                    // View 내부에 있는 Entity는 유일하기 때문에
                    // 해당 엔티티가 소유한 메모리 공간에 대한 데이터 쓰기 또한 data hazard를 발생 시키지 않는다.
                    TransformProxy& proxy = transformFoundItr->second;
                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;

                    const TransformComponent& transformComponent = transformView.get<TransformComponent>(entity);
                    if (const U64 currentDataHashValue = HashInstance(transformComponent);
                        proxy.DataHashValue != currentDataHashValue)
                    {
                        const Matrix transformMat{transformComponent.CreateTransformation()};
                        proxy.GpuData = TransformGpuData{
                            .Cols{
                                {transformMat.m[0][0], transformMat.m[1][0], transformMat.m[2][0], transformMat.m[3][0]},
                                {transformMat.m[0][1], transformMat.m[1][1], transformMat.m[2][1], transformMat.m[3][1]},
                                {transformMat.m[0][2], transformMat.m[1][2], transformMat.m[2][2], transformMat.m[3][2]},
                            }};
                        proxy.DataHashValue = currentDataHashValue;
                        transformProxyPackage.PendingReplicationGroups[workerId].emplace_back(entity);
                    }
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : transformProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    transformProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (auto& [entity, proxy] : entityProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        transformProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : transformProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = transformProxyPackage.ProxyMap.extract(entity);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                transformProxyPackage.PendingDestructions.clear();
            });

        updateTransformProxy.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);
    }

    void SceneProxy::UpdateLightProxy(tf::Subflow& subflow, const Registry& registry)
    {
        IG_CHECK(lightProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = lightProxyPackage.ProxyMap;
        auto& storage = *lightProxyPackage.Storage;
        const auto lightView = registry.view<const LightComponent, const TransformComponent>();

        tf::Task updateLightProxy = subflow.for_each(
            lightView.begin(), lightView.end(),
            [this, &registry, &entityProxyMap, lightView](const Entity entity)
            {
                const Index workerId = taskExecutor->this_worker_id();
                const auto lightItr = entityProxyMap.find(entity);
                if (lightItr == entityProxyMap.end())
                {
                    lightProxyPackage.PendingProxyGroups[workerId].emplace_back(entity, LightProxy{});
                    lightProxyPackage.PendingReplicationGroups[workerId].emplace_back(entity);
                }
                else
                {
                    // View 내부에 있는 Entity는 유일하기 때문에
                    // 해당 엔티티가 소유한 메모리 공간에 대한 데이터 쓰기 또한 data hazard를 발생 시키지 않는다.
                    LightProxy& proxy = lightItr->second;
                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;

                    const auto& lightComponent = lightView.get<LightComponent>(entity);
                    const auto& transformComponent = lightView.get<TransformComponent>(entity);
                    const std::pair<LightComponent, TransformComponent> combinedComponents =
                        std::make_pair(lightComponent, transformComponent);
                    if (const U64 currentDataHashValue = HashInstance(combinedComponents);
                        proxy.DataHashValue != currentDataHashValue)
                    {
                        proxy.GpuData.Property = lightComponent.Property;
                        proxy.GpuData.WorldPosition = transformComponent.Position;
                        proxy.GpuData.Forward = transformComponent.GetForward();

                        proxy.DataHashValue = currentDataHashValue;
                        lightProxyPackage.PendingReplicationGroups[workerId].emplace_back(entity);
                    }
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : lightProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    lightProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (auto& [entity, proxy] : entityProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        lightProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : lightProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = lightProxyPackage.ProxyMap.extract(entity);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                lightProxyPackage.PendingDestructions.clear();
            });

        updateLightProxy.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);
    }

    void SceneProxy::BuildInstancingData(tf::Subflow& subflow, const Registry& registry)
    {
        const auto staticMeshEntityView = registry.view<const StaticMeshComponent, const MaterialComponent, const TransformComponent>();

        instancingPackage.GlobalInstancingMap.clear();
        instancingPackage.NumInstances = 0;

        tf::Task buildLocalInstancingDataMaps = subflow.for_each(
            staticMeshEntityView.begin(), staticMeshEntityView.end(),
            [this, staticMeshEntityView](const Entity entity)
            {
                IG_CHECK(entity != NullEntity);
                const StaticMeshComponent& staticMeshComponent = staticMeshEntityView.get<const StaticMeshComponent>(entity);
                if (!staticMeshComponent.Mesh)
                {
                    return;
                }
                const MaterialComponent& materialComponent = staticMeshEntityView.get<const MaterialComponent>(entity);
                if (!materialComponent.Instance)
                {
                    return;
                }

                const Index workerId = taskExecutor->this_worker_id();
                IG_CHECK(workerId < instancingPackage.ThreadLocalInstancingMaps.size());
                InstancingMap& threadLocalMap = instancingPackage.ThreadLocalInstancingMaps[workerId];

                const MaterialProxy& materialProxy = materialProxyPackage.ProxyMap.at(materialComponent.Instance);
                const MeshProxy& staticMeshProxy = meshProxyPackage.ProxyMap.at(staticMeshComponent.Mesh);
                const U64 instancingKey = InstancingPackage::MakeInstancingMapKey(staticMeshProxy, materialProxy);
                auto foundItr = threadLocalMap.find(instancingKey);
                if (foundItr == threadLocalMap.end())
                {
                    const auto materialDataIdx = (U32)materialProxy.StorageSpace.OffsetIndex;
                    const auto meshDataIdx = (U32)staticMeshProxy.StorageSpace.OffsetIndex;
                    foundItr = threadLocalMap.emplace(instancingKey, InstancingGpuData{materialDataIdx, meshDataIdx}).first;
                }
                IG_CHECK(foundItr != threadLocalMap.end());

                ++foundItr->second.NumInstances;

                localIntermediateStaticMeshData[workerId].emplace_back(entity, instancingKey);
            });

        tf::Task buildGlobalInstancingDataMap = subflow.emplace(
            [this]()
            {
                OrderedInstancingMap& globalInstanceMap = instancingPackage.GlobalInstancingMap;
                for (InstancingMap& localInstanceMap : instancingPackage.ThreadLocalInstancingMaps)
                {
                    for (const auto& [instanceKey, data] : localInstanceMap)
                    {
                        auto foundItr = globalInstanceMap.find(instanceKey);
                        if (foundItr == globalInstanceMap.end())
                        {
                            globalInstanceMap[instanceKey] = data;
                        }
                        else
                        {
                            InstancingGpuData& globalData = foundItr->second;
                            IG_CHECK(globalData.MeshDataIdx == data.MeshDataIdx);
                            IG_CHECK(globalData.MaterialDataIdx == data.MaterialDataIdx);
                            globalData.NumInstances += data.NumInstances;
                        }

                        instancingPackage.NumInstances += data.NumInstances;
                    }

                    localInstanceMap.clear();
                }
            });

        tf::Task allocateSpace = subflow.emplace(
            [this]()
            {
                const Size numInstancing = instancingPackage.GlobalInstancingMap.size();
                if (numInstancing == 0)
                {
                    return;
                }

                instancingPackage.InstancingDataSpace =
                    instancingPackage.InstancingDataStorage->Allocate(numInstancing);
                instancingPackage.IndirectTransformSpace =
                    instancingPackage.IndirectTransformStorage->Allocate(instancingPackage.NumInstances);
            });

        // 결국 transform offset을 정할려면 linear 하게 돌아야 하나?
        tf::Task finalizeGlobalInstancingDataMap = subflow.emplace(
            [this]()
            {
                U32 instancingId = 0;
                U32 transformOffset = 0;
                for (auto& [instanceKey, globalData] : instancingPackage.GlobalInstancingMap)
                {
                    globalData.InstancingId = instancingId;
                    globalData.IndirectTransformOffset = transformOffset;

                    ++instancingId;
                    transformOffset += globalData.NumInstances;
                }

                IG_CHECK(instancingId == instancingPackage.GlobalInstancingMap.size());
                IG_CHECK(transformOffset == instancingPackage.NumInstances);
            });

        buildLocalInstancingDataMaps.precede(buildGlobalInstancingDataMap);
        buildGlobalInstancingDataMap.precede(allocateSpace);
        allocateSpace.precede(finalizeGlobalInstancingDataMap);
    }

    void SceneProxy::UpdateMeshProxy()
    {
        IG_CHECK(meshProxyPackage.PendingReplicationGroups[0].empty());
        IG_CHECK(meshProxyPackage.PendingDestructions.empty());

        auto& proxyMap = meshProxyPackage.ProxyMap;
        auto& storage = *meshProxyPackage.Storage;

        Vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::StaticMesh, true)};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::StaticMesh);
            IG_CHECK(snapshot.IsCached());

            ManagedAsset<StaticMesh> cachedStaticMesh = ManagedAsset<StaticMesh>{snapshot.HandleHash};
            IG_CHECK(cachedStaticMesh);

            if (!proxyMap.contains(cachedStaticMesh))
            {
                proxyMap[cachedStaticMesh] = MeshProxy{.StorageSpace = storage.Allocate(1)};
            }

            MeshProxy& proxy = proxyMap[cachedStaticMesh];
            proxy.bMightBeDestroyed = false;

            const StaticMesh* staticMeshPtr = assetManager->Lookup(cachedStaticMesh);
            IG_CHECK(staticMeshPtr != nullptr);

            if (const U64 currentDataHashValue = HashInstance(*staticMeshPtr);
                proxy.DataHashValue != currentDataHashValue)
            {
                const U8 numLods = staticMeshPtr->GetNumLods();
                Array<MeshLodGpuData, StaticMesh::kMaxNumLods> lods;
                for (U8 lod = 0; lod < numLods; ++lod)
                {
                    const MeshStorage::Space<U32>* indexSpace =
                        meshStorage->Lookup(staticMeshPtr->GetIndexSpace(lod));
                    lods[lod] = MeshLodGpuData{
                        .IndexOffset = (U32)(indexSpace != nullptr ? indexSpace->Allocation.OffsetIndex : 0),
                        .NumIndices = (U32)(indexSpace != nullptr ? indexSpace->Allocation.NumElements : 0)};
                }

                const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage->Lookup(staticMeshPtr->GetVertexSpace());
                proxy.GpuData = MeshGpuData{
                    .VertexOffset = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.OffsetIndex : 0),
                    .NumVertices = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.NumElements : 0),
                    .NumLods = numLods,
                    .Lods = lods,
                    .BoundingVolume = ToBoundingSphere(staticMeshPtr->GetSnapshot().LoadDescriptor.AABB)};
                proxy.DataHashValue = currentDataHashValue;
                meshProxyPackage.PendingReplicationGroups[0].emplace_back(cachedStaticMesh);
            }
        }

        for (auto& [material, proxy] : proxyMap)
        {
            if (proxy.bMightBeDestroyed)
            {
                meshProxyPackage.PendingDestructions.emplace_back(material);
            }
        }

        for (const ManagedAsset<StaticMesh> mesh : meshProxyPackage.PendingDestructions)
        {
            const auto extractedElement = meshProxyPackage.ProxyMap.extract(mesh);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        meshProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateMaterialProxy()
    {
        IG_CHECK(materialProxyPackage.PendingReplicationGroups[0].empty());
        IG_CHECK(materialProxyPackage.PendingDestructions.empty());

        auto& proxyMap = materialProxyPackage.ProxyMap;
        auto& storage = *materialProxyPackage.Storage;

        Vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::Material, true)};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::Material);
            IG_CHECK(snapshot.IsCached());

            ManagedAsset<Material> cachedMaterial{snapshot.HandleHash};
            IG_CHECK(cachedMaterial);

            if (!proxyMap.contains(cachedMaterial))
            {
                proxyMap[cachedMaterial] = MaterialProxy{.StorageSpace = storage.Allocate(1)};
            }

            MaterialProxy& proxy = proxyMap[cachedMaterial];
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
                materialProxyPackage.PendingReplicationGroups[0].emplace_back(cachedMaterial);
            }
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
            const auto extractedElement = materialProxyPackage.ProxyMap.extract(material);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        materialProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateRenderableProxy(tf::Subflow& subflow)
    {
        IG_CHECK(renderableProxyPackage.PendingDestructions.empty());

        auto& renderableProxyMap = renderableProxyPackage.ProxyMap;
        auto& renderableStorage = *renderableProxyPackage.Storage;
        const auto& transformProxyMap = transformProxyPackage.ProxyMap;

        tf::Task buildStaticMeshRenderables = subflow.for_each_index(
            0, (I32)numWorker, 1,
            [this, &renderableProxyMap, &transformProxyMap](const Index workerId)
            {
                Vector<StaticMeshRenderableData>& intermediateData = localIntermediateStaticMeshData[workerId];
                for (const StaticMeshRenderableData& renderableData : intermediateData)
                {
                    const U32 instancingId = instancingPackage.GlobalInstancingMap[renderableData.InstancingKey].InstancingId;
                    const auto renderableFoundItr = renderableProxyMap.find(renderableData.Owner);
                    if (renderableFoundItr == renderableProxyMap.end())
                    {
                        renderableProxyPackage.PendingProxyGroups[taskExecutor->this_worker_id()].emplace_back(
                            renderableData.Owner,
                            RenderableProxy{
                                .StorageSpace = {},
                                .GpuData = RenderableGpuData{
                                    .Type = ERenderableType::StaticMesh,
                                    .DataIdx = instancingId,
                                    .TransformDataIdx = (U32)transformProxyMap.at(renderableData.Owner).StorageSpace.OffsetIndex},
                                .bMightBeDestroyed = false});

                        renderableProxyPackage.PendingReplicationGroups[workerId].emplace_back(renderableData.Owner);
                    }
                    else
                    {
                        RenderableProxy& proxy = renderableFoundItr->second;
                        if (proxy.GpuData.Type != ERenderableType::StaticMesh)
                        {
                            return;
                        }

                        IG_CHECK(proxy.bMightBeDestroyed);
                        proxy.bMightBeDestroyed = false;

                        if (instancingId != proxy.GpuData.DataIdx)
                        {
                            proxy.GpuData.DataIdx = instancingId;
                            renderableProxyPackage.PendingReplicationGroups[workerId].emplace_back(renderableData.Owner);
                        }

                        renderableIndicesGroups[workerId].emplace_back((U32)proxy.StorageSpace.OffsetIndex);
                    }
                }
                intermediateData.clear();
            });

        // 모든 종류의 Renderable에 대한 Proxy가 만들어 진 이후
        tf::Task commitPendingRenderableTask = subflow.emplace(
            [this, &renderableProxyMap, &renderableStorage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    // Commit Pending Renderables
                    for (auto& pendingRenderable : renderableProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingRenderable.second.StorageSpace = renderableStorage.Allocate(1);
                        renderableProxyMap[pendingRenderable.first] = pendingRenderable.second;
                        renderableIndicesGroups[groupIdx].emplace_back((U32)pendingRenderable.second.StorageSpace.OffsetIndex);
                    }
                    renderableProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task commitPendingRenderableIndicesTask = subflow.emplace(
            [this]()
            {
                renderableIndices.clear();
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (const U32 renderableIdx : renderableIndicesGroups[groupIdx])
                    {
                        renderableIndices.emplace_back(renderableIdx);
                    }
                    renderableIndicesGroups[groupIdx].clear();
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

        buildStaticMeshRenderables.precede(commitPendingRenderableTask);
        commitPendingRenderableTask.precede(commitPendingRenderableIndicesTask,
                                            commitPendingDestructionsTask);
    }

    template <typename Proxy, typename Owner>
    void SceneProxy::ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage)
    {
        IG_CHECK(proxyPackage.PendingReplicationGroups.size() == numWorker);
        // 이미 그룹 별로 데이터가 균등 분배 되어있는 상황
        // 필요한 Staging Buffer 크기 == (sum(pendingRepsGroups[0..N].size()) * kDataSize))

        // #sy_todo 더 batching 잘 되도록 개선하기(한번 모은 다음에 재분배 하던가)
        tf::Task prepareReplication = subflow.emplace(
            [this, &proxyPackage, localFrameIdx]()
            {
                Size currentOffset = 0;
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    const Size workGroupDataSize = Proxy::kDataSize * proxyPackage.PendingReplicationGroups[groupIdx].size();
                    proxyPackage.WorkGroupStagingBufferRanges[groupIdx] = std::make_pair(currentOffset, workGroupDataSize);
                    currentOffset += workGroupDataSize;

                    CommandListPool& asyncCopyCmdListPool = renderContext->GetAsyncCopyCommandListPool();
                    if (workGroupDataSize > 0)
                    {
                        proxyPackage.WorkGroupCmdLists[groupIdx] = asyncCopyCmdListPool.Request(localFrameIdx, String(std::format("ProxyRep.{}", groupIdx)));
                    }
                    else
                    {
                        proxyPackage.WorkGroupCmdLists[groupIdx] = nullptr;
                    }
                }

                const Size requiredStagingBufferSize = currentOffset;
                if (proxyPackage.StagingBufferSize[localFrameIdx] < requiredStagingBufferSize)
                {
                    if (proxyPackage.StagingBuffer[localFrameIdx] != nullptr)
                    {
                        proxyPackage.MappedStagingBuffer[localFrameIdx] = nullptr;
                        proxyPackage.StagingBuffer[localFrameIdx]->Unmap();
                        proxyPackage.StagingBuffer[localFrameIdx].reset();
                    }

                    const auto newSize = (U32)std::max(requiredStagingBufferSize, proxyPackage.StagingBufferSize[localFrameIdx] * 2);
                    GpuBufferDesc stagingBufferDesc{};
                    stagingBufferDesc.AsUploadBuffer(newSize);
                    proxyPackage.StagingBuffer[localFrameIdx] =
                        MakePtr<GpuBuffer>(renderContext->GetGpuDevice().CreateBuffer(stagingBufferDesc).value());
                    proxyPackage.MappedStagingBuffer[localFrameIdx] =
                        proxyPackage.StagingBuffer[localFrameIdx]->Map();
                    proxyPackage.StagingBufferSize[localFrameIdx] = newSize;
                }
            });

        // SceneProxy가 Storage를 소유하고 있기때문에 작업 실행 중에 해제되지 않음이 보장됨.
        const RenderHandle<GpuBuffer> storageBuffer = proxyPackage.Storage->GetGpuBuffer();
        GpuBuffer* storageBufferPtr = renderContext->Lookup(storageBuffer);
        IG_CHECK(storageBufferPtr != nullptr);
        tf::Task recordReplicateDataCmd = subflow.for_each_index(
            0, (I32)numWorker, 1,
            [this, &proxyPackage, storageBufferPtr, localFrameIdx](int groupIdx)
            {
                Vector<Owner>& pendingReplications = proxyPackage.PendingReplicationGroups[groupIdx];
                if (pendingReplications.empty())
                {
                    return;
                }

                Vector<typename Proxy::UploadInfo> uploadInfos;
                uploadInfos.reserve(pendingReplications.size());
                for (const Owner owner : pendingReplications)
                {
                    IG_CHECK(proxyPackage.ProxyMap.contains(owner));

                    const Proxy& proxy = proxyPackage.ProxyMap[owner];
                    uploadInfos.emplace_back(
                        typename Proxy::UploadInfo{
                            .StorageSpaceRef = CRef{proxy.StorageSpace},
                            .GpuDataRef = CRef{proxy.GpuData}});
                }
                IG_CHECK(!uploadInfos.empty());

                std::sort(
                    uploadInfos.begin(), uploadInfos.end(),
                    [](const Proxy::UploadInfo& lhs, const Proxy::UploadInfo& rhs)
                    {
                        return lhs.StorageSpaceRef.get().Offset < rhs.StorageSpaceRef.get().Offset;
                    });

                IG_CHECK(proxyPackage.StagingBuffer[localFrameIdx] != nullptr);
                IG_CHECK(storageBufferPtr != nullptr);
                IG_CHECK(proxyPackage.MappedStagingBuffer[localFrameIdx] != nullptr);

                const auto [stagingBufferOffset, allocatedSize] = proxyPackage.WorkGroupStagingBufferRanges[groupIdx];

                auto* mappedUploadBuffer = reinterpret_cast<Proxy::GpuData_t*>(
                    proxyPackage.MappedStagingBuffer[localFrameIdx] + stagingBufferOffset);

                Size srcOffset = stagingBufferOffset;
                Size copySize = 0;
                Size dstOffset = uploadInfos[0].StorageSpaceRef.get().Offset;

                CommandList* cmdList = proxyPackage.WorkGroupCmdLists[groupIdx];
                IG_CHECK(cmdList != nullptr);

                cmdList->Open();
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
                        cmdList->CopyBuffer(*proxyPackage.StagingBuffer[localFrameIdx], srcOffset, copySize, *storageBufferPtr, dstOffset);
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
                cmdList->Close();

                pendingReplications.clear();
            });

        tf::Task submitCmdList = subflow.emplace(
            [this, &proxyPackage]()
            {
                Vector<CommandList*> compactedCmdLists;
                compactedCmdLists.reserve(proxyPackage.WorkGroupCmdLists.size());
                for (CommandList* cmdListPtr : proxyPackage.WorkGroupCmdLists)
                {
                    if (cmdListPtr != nullptr)
                    {
                        compactedCmdLists.emplace_back(cmdListPtr);
                    }
                }

                if (compactedCmdLists.empty())
                {
                    return;
                }

                CommandQueue& cmdQueue = renderContext->GetAsyncCopyQueue();
                IG_CHECK(cmdQueue.GetType() == EQueueType::Copy);
                cmdQueue.ExecuteCommandLists(std::span{compactedCmdLists.data(),
                                                       compactedCmdLists.size()});
            });

        prepareReplication.precede(recordReplicateDataCmd);
        recordReplicateDataCmd.precede(submitCmdList);
    }

    void SceneProxy::ReplicateInstancingData(const LocalFrameIndex localFrameIdx)
    {
        const Size requiredStagingBufferSize = sizeof(InstancingGpuData) * instancingPackage.GlobalInstancingMap.size();
        if (requiredStagingBufferSize == 0)
        {
            return;
        }

        if (instancingPackage.StagingBufferSize[localFrameIdx] < requiredStagingBufferSize)
        {
            if (instancingPackage.StagingBuffer[localFrameIdx] != nullptr)
            {
                instancingPackage.StagingBuffer[localFrameIdx]->Unmap();
                instancingPackage.MappedStagingBuffer[localFrameIdx] = nullptr;
                instancingPackage.StagingBuffer[localFrameIdx].reset();
            }

            const Size newStagingBufferSize = std::max(instancingPackage.StagingBufferSize[localFrameIdx] * 2, requiredStagingBufferSize);
            GpuBufferDesc stagingBufferDesc{};
            stagingBufferDesc.AsUploadBuffer((U32)newStagingBufferSize);
            stagingBufferDesc.DebugName = String(std::format("StaticMeshInstanceStagingBuffer.{}", localFrameIdx));

            GpuDevice& gpuDevice = renderContext->GetGpuDevice();
            instancingPackage.StagingBuffer[localFrameIdx] =
                MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(stagingBufferDesc).value());
            instancingPackage.StagingBufferSize[localFrameIdx] = newStagingBufferSize;

            instancingPackage.MappedStagingBuffer[localFrameIdx] = instancingPackage.StagingBuffer[localFrameIdx]->Map(0);
        }
        IG_CHECK(instancingPackage.StagingBuffer[localFrameIdx] != nullptr);

        auto* mappedStagingBuffer =
            reinterpret_cast<InstancingGpuData*>(instancingPackage.MappedStagingBuffer[localFrameIdx]);
        IG_CHECK(mappedStagingBuffer != nullptr);
        Index instancingIdx = 0;
        for (const auto& [instanceKey, data] : instancingPackage.GlobalInstancingMap)
        {
            mappedStagingBuffer[instancingIdx] = data;
            ++instancingIdx;
        }

        GpuBuffer* instanceStorageBuffer =
            renderContext->Lookup(instancingPackage.InstancingDataStorage->GetGpuBuffer());
        IG_CHECK(instanceStorageBuffer != nullptr);

        CommandListPool& copyCmdListPool = renderContext->GetAsyncCopyCommandListPool();
        auto cmdList = copyCmdListPool.Request(localFrameIdx, "ReplicateInstancingDataCmdList"_fs);
        cmdList->Open();

        cmdList->AddPendingBufferBarrier(*instancingPackage.StagingBuffer[localFrameIdx],
                                         D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                                         D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
        cmdList->AddPendingBufferBarrier(*instanceStorageBuffer,
                                         D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
                                         D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
        cmdList->FlushBarriers();

        cmdList->CopyBuffer(*instancingPackage.StagingBuffer[localFrameIdx], 0, requiredStagingBufferSize,
                            *instanceStorageBuffer, 0);

        cmdList->AddPendingBufferBarrier(*instancingPackage.StagingBuffer[localFrameIdx],
                                         D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_NONE,
                                         D3D12_BARRIER_ACCESS_COPY_SOURCE, D3D12_BARRIER_ACCESS_NO_ACCESS);
        cmdList->AddPendingBufferBarrier(*instanceStorageBuffer,
                                         D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_NONE,
                                         D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_NO_ACCESS);
        cmdList->FlushBarriers();

        cmdList->Close();

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        CommandList* cmdLists[]{cmdList};
        asyncCopyQueue.ExecuteCommandLists(cmdLists);
    }

    void SceneProxy::ReplicateRenderableIndices(const LocalFrameIndex localFrameIdx)
    {
        const Size requiredStagingBufferSize = sizeof(U32) * renderableIndices.size();
        if (requiredStagingBufferSize == 0)
        {
            return;
        }

        if (renderableIndicesStagingBufferSize[localFrameIdx] < requiredStagingBufferSize)
        {
            if (renderableIndicesStagingBuffer[localFrameIdx])
            {
                GpuBuffer* oldStagingBufferPtr = renderContext->Lookup(renderableIndicesStagingBuffer[localFrameIdx]);
                oldStagingBufferPtr->Unmap();
                mappedRenderableIndicesStagingBuffer[localFrameIdx] = nullptr;
                renderContext->DestroyBuffer(renderableIndicesStagingBuffer[localFrameIdx]);
            }

            const Size newStagingBufferSize = std::max(renderableIndicesStagingBufferSize[localFrameIdx] * 2, requiredStagingBufferSize);
            GpuBufferDesc stagingBufferDesc{};
            stagingBufferDesc.AsUploadBuffer((U32)newStagingBufferSize);
            stagingBufferDesc.DebugName = String(std::format("RenderableIndicesStagingBuffer.{}", localFrameIdx));
            renderableIndicesStagingBuffer[localFrameIdx] = renderContext->CreateBuffer(stagingBufferDesc);
            renderableIndicesStagingBufferSize[localFrameIdx] = newStagingBufferSize;

            GpuBuffer* stagingBufferPtr = renderContext->Lookup(renderableIndicesStagingBuffer[localFrameIdx]);
            mappedRenderableIndicesStagingBuffer[localFrameIdx] = stagingBufferPtr->Map(0);
        }
        IG_CHECK(renderableIndicesStagingBuffer[localFrameIdx]);

        const U32 numRenderables = (U32)renderableIndices.size();
        if (numRenderables == 0)
        {
            return;
        }

        if (renderableIndicesBufferSize < numRenderables)
        {
            renderContext->DestroyGpuView(renderableIndicesBufferSrv);
            renderContext->DestroyBuffer(renderableIndicesBuffer);

            const U32 newBufferSize = std::max(numRenderables, renderableIndicesBufferSize * 2);
            GpuBufferDesc renderableIndicesBufferDesc;
            renderableIndicesBufferDesc.AsStructuredBuffer<U32>(newBufferSize);
            renderableIndicesBufferDesc.DebugName = String(std::format("RenderableIndicesBuffer"));
            renderableIndicesBuffer = renderContext->CreateBuffer(renderableIndicesBufferDesc);
            renderableIndicesBufferSrv = renderContext->CreateShaderResourceView(renderableIndicesBuffer);

            renderableIndicesBufferSize = newBufferSize;
        }

        GpuBuffer* renderableIndicesBufferPtr = renderContext->Lookup(renderableIndicesBuffer);
        IG_CHECK(renderableIndicesBufferPtr != nullptr);

        GpuBuffer* stagingBufferPtr = renderContext->Lookup(renderableIndicesStagingBuffer[localFrameIdx]);
        IG_CHECK(stagingBufferPtr != nullptr);

        IG_CHECK(mappedRenderableIndicesStagingBuffer[localFrameIdx] != nullptr);
        auto* mappedBuffer = mappedRenderableIndicesStagingBuffer[localFrameIdx];

        const U32 replicationSize = (U32)requiredStagingBufferSize;
        IG_CHECK(replicationSize == (sizeof(U32) * numRenderables));
        std::memcpy(mappedBuffer, renderableIndices.data(), replicationSize);

        CommandListPool& copyCmdListPool = renderContext->GetAsyncCopyCommandListPool();
        auto cmdList = copyCmdListPool.Request(localFrameIdx, "ReplicateRenderableIndicesCmdList"_fs);
        cmdList->Open();
        cmdList->CopyBuffer(*stagingBufferPtr, 0, replicationSize, *renderableIndicesBufferPtr, 0);
        cmdList->Close();

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        CommandList* cmdLists[]{cmdList};
        asyncCopyQueue.ExecuteCommandLists(cmdLists);
    }
} // namespace ig
