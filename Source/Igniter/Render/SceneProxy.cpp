#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/UnifiedMeshStorage.h"
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
    SceneProxy::SceneProxy(tf::Executor& taskExecutor, RenderContext& renderContext, AssetManager& assetManager)
        : taskExecutor(&taskExecutor)
        , renderContext(&renderContext)
        , assetManager(&assetManager)
        , numWorker((U32)taskExecutor.num_workers())
        , lightProxyPackage(renderContext, GpuStorageDesc{String(std::format("GpuLightDataStorage(SceneProxy)")), LightProxy::kDataSize, kNumInitLightElements}, numWorker)
        , materialProxyPackage(renderContext, GpuStorageDesc{String(std::format("GpuMaterialStorage(SceneProxy)")), MaterialProxy::kDataSize, kNumInitMaterialElements}, numWorker)
        , staticMeshProxyPackage(renderContext, GpuStorageDesc{String(std::format("GpuStaticMeshStorage(SceneProxy)")), MeshProxy::kDataSize, kNumInitMeshProxies}, numWorker)
        , meshInstanceProxyPackage(renderContext, GpuStorageDesc{String(std::format("GpuMeshInstanceStorage(SceneProxy)")), MeshInstanceProxy::kDataSize, kNumInitMeshProxies}, numWorker)
    {
    }

    SceneProxy::~SceneProxy()
    {
    }

    GpuSyncPoint SceneProxy::Replicate(const LocalFrameIndex localFrameIdx, const World& world)
    {
        IG_CHECK(taskExecutor != nullptr);
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        ZoneScoped;

        if (invalidationFuture[localFrameIdx].valid())
        {
            invalidationFuture[localFrameIdx].wait();
        }

        const Registry& registry = world.GetRegistry();

        tf::Taskflow rootTaskFlow{};

        tf::Task updateLightTask = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateLightProxy");
                UpdateLightProxy(subflow, registry);
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

        tf::Task updateStaticMeshTask = rootTaskFlow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateStaticMeshProxy");
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateStaticMeshProxy(subflow);
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            });

        tf::Task updateSkeletalMeshTask = rootTaskFlow.emplace(
            [this]([[maybe_unused]] tf::Subflow& subflow) {});

        tf::Task updateMeshInstanceTask = rootTaskFlow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("UpdateMeshInstanceProxy");
                UpdateMeshInstanceProxy(subflow, registry);
            });

        updateMeshInstanceTask.succeed(updateMaterialTask, updateStaticMeshTask, updateSkeletalMeshTask);

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

        tf::Task replicateStaticMeshData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateStaticMeshData");
                ReplicateProxyData(subflow, localFrameIdx, staticMeshProxyPackage);
                subflow.join();
            });

        replicateLightData.succeed(updateLightTask);
        replicateMaterialData.succeed(updateMaterialTask);
        replicateStaticMeshData.succeed(updateStaticMeshTask);

        // 현재 프레임 작업 시작
        taskExecutor->run(rootTaskFlow).wait();
        // 현재 프레임 작업 완료

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        IG_CHECK(asyncCopyQueue.GetType() == EQueueType::Copy);
        GpuSyncPoint syncPoint = asyncCopyQueue.MakeSyncPointWithSignal(renderContext->GetAsyncCopyFence());
        return syncPoint;
    }

    void SceneProxy::PrepareNextFrame(const LocalFrameIndex localFrameIdx)
    {
        // 다음 프레임 작업 준비
        const LocalFrameIndex nextLocalFrameIdx = (localFrameIdx + 1) % NumFramesInFlight;
        tf::Taskflow prepareNextFrameFlow{};
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

        tf::Task invalidateStaticMeshProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameStaticMeshProxy");
                subflow.for_each(
                    staticMeshProxyPackage.ProxyMap.begin(), staticMeshProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        tf::Task invalidateMaterialProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameMaterialProxy");
                subflow.for_each(
                    materialProxyPackage.ProxyMap.begin(), materialProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        tf::Task invalidateMeshInstanceProxy = prepareNextFrameFlow.emplace(
            [this, nextLocalFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("InvalidateNextFrameMeshInstProxy");
                subflow.for_each(
                    meshInstanceProxyPackage.ProxyMap.begin(), meshInstanceProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        invalidationFuture[nextLocalFrameIdx] = taskExecutor->run(std::move(prepareNextFrameFlow));
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

        subflow.join();
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

            Handle<Material> cachedMaterial{snapshot.HandleHash};
            IG_CHECK(cachedMaterial);

            if (!proxyMap.contains(cachedMaterial))
            {
                proxyMap[cachedMaterial] = MaterialProxy{.StorageSpace = storage.Allocate(1)};
            }

            MaterialProxy& proxy = proxyMap[cachedMaterial];
            proxy.bMightBeDestroyed = false;

            const Material* materialPtr = assetManager->Lookup(cachedMaterial);
            IG_CHECK(materialPtr != nullptr);

            GpuMaterial newData{};
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

        for (const Handle<Material> material : materialProxyPackage.PendingDestructions)
        {
            const auto extractedElement = materialProxyPackage.ProxyMap.extract(material);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        materialProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateStaticMeshProxy(tf::Subflow& subflow)
    {
        IG_CHECK(staticMeshProxyPackage.PendingDestructions.empty());

        auto& proxyMap = staticMeshProxyPackage.ProxyMap;
        auto& storage = *staticMeshProxyPackage.Storage;

        Vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::StaticMesh, true)};
        tf::Task updateStaticMeshProxy = subflow.for_each(
            snapshots.begin(), snapshots.end(),
            [this, &proxyMap](const AssetManager::Snapshot& snapshot)
            {
            const Index workerId = taskExecutor->this_worker_id();
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::StaticMesh);
            IG_CHECK(snapshot.IsCached());

            Handle<StaticMesh> cachedStaticMesh = Handle<StaticMesh>{snapshot.HandleHash};
            IG_CHECK(cachedStaticMesh);

            const auto staticMeshItr = proxyMap.find(cachedStaticMesh);
            if (staticMeshItr == proxyMap.end())
            {
                staticMeshProxyPackage.PendingProxyGroups[workerId].emplace_back(cachedStaticMesh, MeshProxy{});
            }
            else
            {
                const UnifiedMeshStorage& unifiedMeshStorage = renderContext->GetUnifiedMeshStorage();

                MeshProxy& proxy = staticMeshItr->second;
                proxy.bMightBeDestroyed = false;

                const StaticMesh* staticMeshPtr = assetManager->Lookup(cachedStaticMesh);
                IG_CHECK(staticMeshPtr != nullptr);

                if (const U64 currentDataHashValue = HashInstance(*staticMeshPtr);
                    proxy.DataHashValue != currentDataHashValue)
                {
                    const Mesh& mesh = staticMeshPtr->GetMesh();

                    const MeshVertexAllocation* vertexAllocPtr = unifiedMeshStorage.Lookup(mesh.VertexStorageAlloc);
                    IG_CHECK(vertexAllocPtr != nullptr);
                    proxy.GpuData.VertexStorageByteOffset = (U32)vertexAllocPtr->Alloc.Offset;
                    proxy.GpuData.NumLevelOfDetails = mesh.NumLevelOfDetails;
                    for (U8 lod = 0; lod < proxy.GpuData.NumLevelOfDetails; ++lod)
                    {
                        const MeshLod& meshLod = mesh.LevelOfDetails[lod];
                        const GpuStorage::Allocation* indexStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.IndexStorageAlloc);
                        IG_CHECK(indexStorageAllocPtr != nullptr);
                        const GpuStorage::Allocation* triangleStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.TriangleStorageAlloc);
                        IG_CHECK(triangleStorageAllocPtr != nullptr);
                        const GpuStorage::Allocation* meshletStorageAllocPtr = unifiedMeshStorage.Lookup(meshLod.MeshletStorageAlloc);
                        IG_CHECK(meshletStorageAllocPtr != nullptr);

                        GpuMeshLod& gpuMeshLod = proxy.GpuData.LevelOfDetails[lod];
                        gpuMeshLod.IndexStorageOffset = (U32)indexStorageAllocPtr->OffsetIndex;
                        gpuMeshLod.TriangleStorageOffset = (U32)triangleStorageAllocPtr->OffsetIndex;
                        gpuMeshLod.MeshletStorageOffset = (U32)meshletStorageAllocPtr->OffsetIndex;
                        gpuMeshLod.NumMeshlets = (U32)meshletStorageAllocPtr->NumElements;
                    }
                    proxy.GpuData.MeshBoundingSphere = ToBoundingSphere(mesh.BoundingBox);

                    proxy.DataHashValue = currentDataHashValue;
                    staticMeshProxyPackage.PendingReplicationGroups[workerId].emplace_back(cachedStaticMesh);
                }
            } });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &proxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (auto& [pendingHandle, pendingProxy] : staticMeshProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        proxyMap[pendingHandle] = pendingProxy;
                    }
                    staticMeshProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &proxyMap, &storage]()
            {
                for (auto& [handle, proxy] : proxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        staticMeshProxyPackage.PendingDestructions.emplace_back(handle);
                    }
                }

                for (const Handle<StaticMesh> handle : staticMeshProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = staticMeshProxyPackage.ProxyMap.extract(handle);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                staticMeshProxyPackage.PendingDestructions.clear();
            });

        updateStaticMeshProxy.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);

        subflow.join();
    }

    void SceneProxy::UpdateMeshInstanceProxy(tf::Subflow& subflow, const Registry& registry)
    {
        IG_CHECK(meshInstanceProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = meshInstanceProxyPackage.ProxyMap;
        auto& storage = *meshInstanceProxyPackage.Storage;

        const auto staticMeshView = registry.view<const TransformComponent, const StaticMeshComponent, const MaterialComponent>();
        tf::Task updateStaticMeshInstances = subflow.for_each(
            staticMeshView.begin(), staticMeshView.end(),
            [this, &entityProxyMap, staticMeshView](const Entity entity)
            {
                const Index workerId = taskExecutor->this_worker_id();
                auto& pendingProxyGroup = meshInstanceProxyPackage.PendingProxyGroups[workerId];
                auto& pendingRepGroup = meshInstanceProxyPackage.PendingReplicationGroups[workerId];
                const auto& materialProxyMap = materialProxyPackage.ProxyMap;
                const auto& staticMeshProxyMap = staticMeshProxyPackage.ProxyMap;

                const auto meshInstanceProxyItr = entityProxyMap.find(entity);
                if (meshInstanceProxyItr == entityProxyMap.end())
                {
                    pendingProxyGroup.emplace_back(entity, MeshInstanceProxy{});
                }
                else
                {
                    MeshInstanceProxy& proxy = meshInstanceProxyItr->second;
                    IG_CHECK(proxy.bMightBeDestroyed);
                    proxy.bMightBeDestroyed = false;

                    const StaticMeshComponent& staticMeshComponent = staticMeshView.get<const StaticMeshComponent>(entity);
                    if (!staticMeshComponent.Mesh)
                    {
                        return;
                    }
                    const MaterialComponent& materialComponent = staticMeshView.get<const MaterialComponent>(entity);
                    if (!materialComponent.Instance)
                    {
                        return;
                    }

                    const TransformComponent& transform = staticMeshView.get<const TransformComponent>(entity);
                    if (const U64 currentHashVal = HashInstances(transform, staticMeshComponent, materialComponent);
                        proxy.DataHashValue != currentHashVal)
                    {
                        const Matrix transformMat{transform.CreateTransformation()};
                        proxy.GpuData.ToWorld[0] = {transformMat.m[0][0], transformMat.m[1][0], transformMat.m[2][0], transformMat.m[3][0]};
                        proxy.GpuData.ToWorld[1] = {transformMat.m[0][1], transformMat.m[1][1], transformMat.m[2][1], transformMat.m[3][1]};
                        proxy.GpuData.ToWorld[2] = {transformMat.m[0][2], transformMat.m[1][2], transformMat.m[2][2], transformMat.m[3][2]};

                        const MeshProxy& meshProxy = staticMeshProxyMap.at(staticMeshComponent.Mesh);
                        const MaterialProxy& materialProxy = materialProxyMap.at(materialComponent.Instance);

                        proxy.GpuData.MeshType = EMeshType::Static;
                        proxy.GpuData.MeshProxyIdx = (U32)meshProxy.StorageSpace.OffsetIndex;
                        proxy.GpuData.MaterialProxyIdx = (U32)materialProxy.StorageSpace.OffsetIndex;
                        pendingRepGroup.emplace_back(entity);
                    }
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : meshInstanceProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    meshInstanceProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (auto& [entity, proxy] : entityProxyMap)
                {
                    if (proxy.bMightBeDestroyed)
                    {
                        meshInstanceProxyPackage.PendingDestructions.emplace_back(entity);
                    }
                }

                for (const Entity entity : meshInstanceProxyPackage.PendingDestructions)
                {
                    const auto extractedElement = meshInstanceProxyPackage.ProxyMap.extract(entity);
                    IG_CHECK(extractedElement.has_value());
                    IG_CHECK(extractedElement->second.StorageSpace.IsValid());
                    storage.Deallocate(extractedElement->second.StorageSpace);
                }
                meshInstanceProxyPackage.PendingDestructions.clear();
            });

        updateStaticMeshInstances.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);

        subflow.join();
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
        const Handle<GpuBuffer> storageBuffer = proxyPackage.Storage->GetGpuBuffer();
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
} // namespace ig
