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
        assetManager(&assetManager),
        numWorker((U32)Engine::GetTaskExecutor().num_workers())
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

            meshProxyPackage.Storage[localFrameIdx] =
                MakePtr<GpuStorage>(
                    renderContext,
                    String(std::format("MeshDataStorage{}", localFrameIdx)),
                    MeshProxy::kDataSize,
                    kNumInitMeshElements);

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

        staticMeshProxyPackage.PendingReplicationGroups.resize(numWorker);
        staticMeshProxyPackage.PendingProxyGroups.resize(numWorker);
        staticMeshProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        staticMeshProxyPackage.WorkGroupCmdLists.resize(numWorker);

        lightProxyPackage.PendingReplicationGroups.resize(numWorker);

        renderableProxyPackage.PendingReplicationGroups.resize(numWorker);
        renderableProxyPackage.PendingProxyGroups.resize(numWorker);
        renderableProxyPackage.WorkGroupStagingBufferRanges.resize(numWorker);
        renderableProxyPackage.WorkGroupCmdLists.resize(numWorker);

        renderableIndicesGroups.resize(numWorker);
        renderableIndices.reserve(kNumInitRenderableIndices);
        lightIndices.reserve(kNumInitLightIndices);
    }

    SceneProxy::~SceneProxy()
    {
        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            transformProxyPackage.Storage[localFrameIdx]->ForceReset();
            materialProxyPackage.Storage[localFrameIdx]->ForceReset();
            meshProxyPackage.Storage[localFrameIdx]->ForceReset();
            staticMeshProxyPackage.Storage[localFrameIdx]->ForceReset();
            renderableProxyPackage.Storage[localFrameIdx]->ForceReset();
            lightProxyPackage.Storage[localFrameIdx]->ForceReset();

            renderContext->DestroyGpuView(renderableIndicesBufferSrv[localFrameIdx]);
            renderContext->DestroyGpuView(lightIndicesBufferSrv[localFrameIdx]);

            renderContext->DestroyBuffer(renderableIndicesBuffer[localFrameIdx]);
            renderContext->DestroyBuffer(lightIndicesBuffer[localFrameIdx]);

            renderContext->DestroyBuffer(renderableIndicesStagingBuffer[localFrameIdx]);
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

        tf::Task updateMeshTask = rootTaskFlow.emplace(
            [this, localFrameIdx]()
            {
                ZoneScopedN("UpdateMeshProxy");
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateMeshProxy(localFrameIdx);
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

        updateStaticMeshTask.succeed(updateMaterialTask, updateMeshTask);
        updateRenderableTask.succeed(updateTransformTask, updateStaticMeshTask);

        tf::Task replicateTransformData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateTransformData");
                ReplicateProxyData(subflow, localFrameIdx, transformProxyPackage);
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

        tf::Task replicateStaticMeshData = rootTaskFlow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("ReplicateStaticMeshData");
                ReplicateProxyData(subflow, localFrameIdx, staticMeshProxyPackage);
                subflow.join();
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
        replicateMaterialData.succeed(updateMaterialTask);
        replicateMeshData.succeed(updateMeshTask);
        replicateStaticMeshData.succeed(updateStaticMeshTask);
        //// 이하 두개 태스크는 추후 렌더러블로 다뤄질 수 있는 종류가 많아 질수록 종속성을 추가해야함.
        replicateRenderableData.succeed(updateRenderableTask);
        replicateRenderableIndices.succeed(updateRenderableTask);

        executor.run(rootTaskFlow).wait();

        CommandQueue& asyncCopyQueue = renderContext->GetAsyncCopyQueue();
        IG_CHECK(asyncCopyQueue.GetType() == EQueueType::Copy);
        GpuSyncPoint syncPoint = asyncCopyQueue.MakeSyncPointWithSignal(renderContext->GetAsyncCopyFence());
        return syncPoint;
    }

    void SceneProxy::UpdateTransformProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        IG_CHECK(transformProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];
        auto invalidateProxyTask = subflow.for_each(
            entityProxyMap.begin(), entityProxyMap.end(),
            [](auto& keyValuePair)
            {
                keyValuePair.second.bMightBeDestroyed = true;
            });

        auto& storage = *transformProxyPackage.Storage[localFrameIdx];
        const auto transformView = registry.view<const TransformComponent>();

        tf::Task createNewProxyTask = subflow.for_each(
            transformView.begin(), transformView.end(),
            [this, &registry, &entityProxyMap, transformView](const Entity entity)
            {
                if (!entityProxyMap.contains(entity))
                {
                    transformProxyPackage.PendingProxyGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back(entity, TransformProxy{});
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

        tf::Task updateProxyTask = subflow.for_each(
            transformView.begin(), transformView.end(),
            [this, &registry, &entityProxyMap, transformView](const Entity entity)
            {
                TransformProxy& proxy = entityProxyMap[entity];
                IG_CHECK(proxy.bMightBeDestroyed);
                proxy.bMightBeDestroyed = false;

                const TransformComponent& transformComponent = registry.get<TransformComponent>(entity);
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

                    transformProxyPackage.PendingReplicationGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back(entity);
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage, localFrameIdx]()
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
        updateProxyTask.precede(commitDestructions);
    }

    void SceneProxy::UpdateMeshProxy(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(meshProxyPackage.PendingReplicationGroups[0].empty());
        IG_CHECK(meshProxyPackage.PendingDestructions.empty());

        auto& proxyMap = meshProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *meshProxyPackage.Storage[localFrameIdx];

        for (auto& [staticMesh, proxy] : proxyMap)
        {
            IG_CHECK(staticMesh);
            proxy.bMightBeDestroyed = true;
        }

        std::vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::StaticMesh, true)};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::StaticMesh);
            IG_CHECK(snapshot.IsCached());

            ManagedAsset<StaticMesh> cachedStaticMesh = assetManager->LoadStaticMesh(snapshot.Info.GetGuid(), true);
            IG_CHECK(cachedStaticMesh);

            if (!proxyMap.contains(cachedStaticMesh))
            {
                proxyMap[cachedStaticMesh] = MeshProxy{.StorageSpace = storage.Allocate(1)};
            }

            MeshProxy& proxy = proxyMap[cachedStaticMesh];
            IG_CHECK(proxy.bMightBeDestroyed);
            proxy.bMightBeDestroyed = false;

            const StaticMesh* staticMeshPtr = assetManager->Lookup(cachedStaticMesh);
            IG_CHECK(staticMeshPtr != nullptr);

            if (const U64 currentDataHashValue = HashInstance(*staticMeshPtr);
                proxy.DataHashValue != currentDataHashValue)
            {
                const MeshStorage::Handle<U32> vertexIndexSpace = staticMeshPtr->GetVertexIndexSpace();
                const MeshStorage::Handle<VertexSM> vertexSpace = staticMeshPtr->GetVertexSpace();
                const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage->Lookup(vertexSpace);
                const MeshStorage::Space<U32>* vertexIndexSpacePtr = meshStorage->Lookup(vertexIndexSpace);
                proxy.GpuData = MeshGpuData{
                    .VertexOffset = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.OffsetIndex : 0),
                    .NumVertices = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.NumElements : 0),
                    .IndexOffset = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.OffsetIndex : 0),
                    .NumIndices = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.NumElements : 0),
                    .BoundingVolume = ToBoundingSphere(staticMeshPtr->GetSnapshot().LoadDescriptor.AABB)};
                proxy.DataHashValue = currentDataHashValue;
                meshProxyPackage.PendingReplicationGroups[0].emplace_back(cachedStaticMesh);
            }

            assetManager->Unload(cachedStaticMesh, true);
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
            const auto extractedElement = meshProxyPackage.ProxyMap[localFrameIdx].extract(mesh);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        materialProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateMaterialProxy(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(materialProxyPackage.PendingReplicationGroups[0].empty());
        IG_CHECK(materialProxyPackage.PendingDestructions.empty());

        auto& proxyMap = materialProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *materialProxyPackage.Storage[localFrameIdx];

        for (auto& [material, proxy] : proxyMap)
        {
            IG_CHECK(material);
            proxy.bMightBeDestroyed = true;
        }

        std::vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots(EAssetCategory::Material, true)};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            IG_CHECK(snapshot.Info.GetCategory() == EAssetCategory::Material);
            IG_CHECK(snapshot.IsCached());

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
                materialProxyPackage.PendingReplicationGroups[0].emplace_back(cachedMaterial);
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
        IG_CHECK(staticMeshProxyPackage.PendingDestructions.empty());

        const auto& materialProxyMap = materialProxyPackage.ProxyMap[localFrameIdx];
        const auto& meshProxyMap = meshProxyPackage.ProxyMap[localFrameIdx];

        auto& entityProxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        tf::Task invalidateProxyTask = subflow.for_each(
            entityProxyMap.begin(), entityProxyMap.end(),
            [](auto& keyValuePair)
            {
                keyValuePair.second.bMightBeDestroyed = true;
            });

        auto& storage = *staticMeshProxyPackage.Storage[localFrameIdx];
        const auto staticMeshEntityView = registry.view<const StaticMeshComponent, const TransformComponent>();

        tf::Task createNewProxyTask = subflow.for_each(
            staticMeshEntityView.begin(), staticMeshEntityView.end(),
            [this, &registry, &entityProxyMap, staticMeshEntityView](const Entity entity)
            {
                const Size groupIdx = Engine::GetTaskExecutor().this_worker_id();
                if (!entityProxyMap.contains(entity))
                {
                    const StaticMeshComponent& staticMeshComponent = staticMeshEntityView.get<const StaticMeshComponent>(entity);
                    if (!staticMeshComponent.Mesh)
                    {
                        return;
                    }

                    staticMeshProxyPackage.PendingProxyGroups[groupIdx].emplace_back(entity, StaticMeshProxy{});
                }
            });

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorker; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : staticMeshProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    staticMeshProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            }); // 여기엔 별도로 딱히 추가 처리는 안해줘도 될 것 같아 보임!

        // 에셋 매니저에서 미리 material 처럼 가져온 다음에
        // static mesh handle - static mesh ptr 맵을 만들고 참조하기 (read만하기)
        staticMeshTable.clear();
        std::vector<AssetManager::Snapshot> staticMeshSnapshots{assetManager->TakeSnapshots(EAssetCategory::StaticMesh, true)};
        for (const AssetManager::Snapshot& staticMeshSnapshot : staticMeshSnapshots)
        {
            IG_CHECK(staticMeshSnapshot.IsCached());
            // 조금 꼼수긴 하지만, 여전히 valid 한 방식이라고 생각한다.
            // 잘못된 핸들은 애초에 Lookup이 불가능하고. 애초에 핸들 자체가 소유권을 가질 핸들 만 제대로
            // 이해하고 해제 해준다면 문제 없기 때문.
            // 성능면에선 실제로 렌더링 될 메시보다 유효한 에셋의 수가 더 적을 것이라고 예측되고
            // Lookup에 의한 mutex-lock의 횟수 또한 현저히 적거나 거의 없을 것이라고 생각된다.
            const ManagedAsset<StaticMesh> reconstructedHandle{staticMeshSnapshot.HandleHash};
            staticMeshTable[reconstructedHandle] = assetManager->Lookup(reconstructedHandle);
        }

        tf::Task updateProxyTask = subflow.for_each(
            staticMeshEntityView.begin(), staticMeshEntityView.end(),
            [this, &registry, &entityProxyMap, &meshProxyMap, &materialProxyMap, staticMeshEntityView](const Entity entity)
            {
                StaticMeshProxy& proxy = entityProxyMap[entity];
                IG_CHECK(proxy.bMightBeDestroyed);
                proxy.bMightBeDestroyed = false;

                const StaticMeshComponent& staticMeshComponent = registry.get<StaticMeshComponent>(entity);
                const StaticMesh* staticMeshPtr = staticMeshTable[staticMeshComponent.Mesh];
                IG_CHECK(staticMeshPtr != nullptr);

                if (const U64 currentDataHashValue = HashInstance(*staticMeshPtr);
                    proxy.DataHashValue != currentDataHashValue)
                {
                    const ManagedAsset<Material> material = staticMeshPtr->GetMaterial();
                    IG_CHECK(materialProxyMap.contains(material));

                    proxy.GpuData = StaticMeshGpuData{
                        .MaterialDataIdx = (U32)materialProxyMap.at(material).StorageSpace.OffsetIndex,
                        .MeshDataIdx = (U32)meshProxyMap.at(staticMeshComponent.Mesh).StorageSpace.OffsetIndex};
                    proxy.DataHashValue = currentDataHashValue;

                    staticMeshProxyPackage.PendingReplicationGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back(entity);
                }
            });

        tf::Task commitDestructions = subflow.emplace(
            [this, &entityProxyMap, &storage, localFrameIdx]()
            {
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
        updateProxyTask.precede(commitDestructions);
    }

    void SceneProxy::UpdateRenderableProxy(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(renderableProxyPackage.PendingDestructions.empty());

        auto& renderableProxyMap = renderableProxyPackage.ProxyMap[localFrameIdx];
        auto& renderableStorage = *renderableProxyPackage.Storage[localFrameIdx];
        const auto& staticMeshProxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        const auto& transformProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];

        tf::Task invalidateProxyTask = subflow.for_each(
            renderableProxyMap.begin(), renderableProxyMap.end(),
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
        // 확장시 2가지 선택 가능한 선택지.
        // 1. 한번에 하나의 타입에 대해서 순차적 처리 => 이 방식을 선제 구현 후 프로파일링 후 추가 조치 할 것!
        // 2. 각각 개별적인 pending list를 가져서 전부 개별적으로 동시적으로 처리
        // (ex. pendingStaticMeshRenderableProxy, pendingSkeletalMeshRenderableProxy, ...etc)
        // Static Mesh
        tf::Task createRenderableStaticMeshTask = subflow.for_each(
            staticMeshProxyMap.cbegin(), staticMeshProxyMap.cend(),
            [this, &renderableProxyMap, &staticMeshProxyMap, &transformProxyMap](const auto& entityProxyPair)
            {
                const Entity entity = entityProxyPair.first;
                IG_CHECK(transformProxyMap.contains(entity));
                if (!renderableProxyMap.contains(entity))
                {
                    renderableProxyPackage.PendingProxyGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back(
                        entity,
                        RenderableProxy{
                            .StorageSpace = {},
                            .GpuData = RenderableGpuData{
                                .Type = ERenderableType::StaticMesh,
                                .DataIdx = (U32)staticMeshProxyMap.at(entity).StorageSpace.OffsetIndex,
                                .TransformDataIdx = (U32)transformProxyMap.at(entity).StorageSpace.OffsetIndex}});

                    renderableProxyPackage.PendingReplicationGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back(entity);
                }
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
                    }
                    renderableProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            });

        // Storage 할당이 끝나면 각 종류의 Renderable들에 대한 proxy 유효성(수명) 평가와 renderable indices 구성
        tf::Task constructStaticMeshRenderableIndicesTask = subflow.for_each(
            staticMeshProxyMap.cbegin(), staticMeshProxyMap.cend(),
            [this, &renderableProxyMap, &staticMeshProxyMap](const auto& entityProxyPair)
            {
                const Entity entity = entityProxyPair.first;
                RenderableProxy& proxy = renderableProxyMap[entity];
                if (proxy.GpuData.Type != ERenderableType::StaticMesh)
                {
                    return;
                }

                IG_CHECK(proxy.bMightBeDestroyed);
                proxy.bMightBeDestroyed = false;
                renderableIndicesGroups[Engine::GetTaskExecutor().this_worker_id()].emplace_back((U32)proxy.StorageSpace.OffsetIndex);
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

        invalidateProxyTask.precede(createRenderableStaticMeshTask);
        createRenderableStaticMeshTask.precede(commitPendingRenderableTask);
        commitPendingRenderableTask.precede(constructStaticMeshRenderableIndicesTask);
        constructStaticMeshRenderableIndicesTask.precede(commitPendingRenderableIndicesTask,
                                                         commitPendingDestructionsTask);
    }

    template <typename Proxy, typename Owner>
    void SceneProxy::ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage)
    {
        IG_CHECK(proxyPackage.PendingReplicationGroups.size() == numWorker);
        // 이미 그룹 별로 데이터가 균등 분배 되어있는 상황
        // 필요한 Staging Buffer 크기 == (sum(pendingRepsGroups[0..N].size()) * kDataSize))

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
        const RenderHandle<GpuBuffer> storageBuffer = proxyPackage.Storage[localFrameIdx]->GetGpuBuffer();
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
                    IG_CHECK(proxyPackage.ProxyMap[localFrameIdx].contains(owner));

                    const Proxy& proxy = proxyPackage.ProxyMap[localFrameIdx][owner];
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

                CommandQueue& cmdQueue = renderContext->GetAsyncCopyQueue();
                IG_CHECK(cmdQueue.GetType() == EQueueType::Copy);
                cmdQueue.ExecuteCommandLists(std::span{compactedCmdLists.data(),
                                                       compactedCmdLists.size()});
            });

        prepareReplication.precede(recordReplicateDataCmd);
        recordReplicateDataCmd.precede(submitCmdList);
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

        GpuBuffer* stagingBufferPtr = renderContext->Lookup(renderableIndicesStagingBuffer[localFrameIdx]);
        IG_CHECK(stagingBufferPtr != nullptr);

        IG_CHECK(mappedRenderableIndicesStagingBuffer[localFrameIdx] != nullptr);
        auto* mappedBuffer = mappedRenderableIndicesStagingBuffer[localFrameIdx];

        const U32 replicationSize = (U32)requiredStagingBufferSize;
        IG_CHECK(replicationSize == (sizeof(U32) * numMaxRenderables[localFrameIdx]));
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
