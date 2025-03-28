#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/GpuStagingBuffer.h"
#include "Igniter/Render/UnifiedMeshStorage.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Asset/Material.h"
#include "Igniter/Asset/StaticMesh.h"
#include "Igniter/Asset/Texture.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/MaterialComponent.h"
#include "Igniter/Component/LightComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Render/SceneProxy.h"

namespace ig
{
    SceneProxy::SceneProxy(tf::Executor& taskExecutor, RenderContext& renderContext, AssetManager& assetManager)
        : taskExecutor(&taskExecutor)
        , renderContext(&renderContext)
        , assetManager(&assetManager)
        , numWorkers((U32)taskExecutor.num_workers())
        , lightProxyPackage(renderContext, GpuStorageDesc{"GpuLightDataStorage(SceneProxy)", LightProxy::kDataSize, kNumInitLightElements}, numWorkers)
        , materialProxyPackage(renderContext, GpuStorageDesc{"GpuMaterialStorage(SceneProxy)", MaterialProxy::kDataSize, kNumInitMaterialElements}, numWorkers)
        , staticMeshProxyPackage(renderContext, GpuStorageDesc{"GpuStaticMeshStorage(SceneProxy)", MeshProxy::kDataSize, kNumInitMeshProxies}, numWorkers)
        , meshInstanceProxyPackage(renderContext, GpuStorageDesc{"GpuMeshInstanceStorage(SceneProxy)", MeshInstanceProxy::kDataSize, kNumInitMeshProxies}, numWorkers)
    {
        ResizeMeshInstanceIndicesBuffer(kInitNumMeshInstanceIndices);

        meshInstanceIndicesUploadInfos.reserve(numWorkers);
        meshInstanceIndicesGroups.resize(numWorkers);
        const Size initNumMeshInstanceIndicesPerWorker = kInitNumMeshInstanceIndices / numWorkers;
        for (Vector<U32>& meshInstanceIndices : meshInstanceIndicesGroups)
        {
            meshInstanceIndices.resize(initNumMeshInstanceIndicesPerWorker);
        }

        GpuBufferDesc gpuConstantsBufferDesc{};
        gpuConstantsBufferDesc.AsConstantBuffer<GpuConstants>();
        gpuConstantsBufferDesc.DebugName = "SceneProxyConstantsBuffer";
        gpuConstantsBufferDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        gpuConstantsBuffer = renderContext.CreateBuffer(gpuConstantsBufferDesc);
        gpuConstantsCbv = renderContext.CreateConstantBufferView(gpuConstantsBuffer);
    }

    SceneProxy::~SceneProxy()
    {
        if (meshInstanceIndicesBuffer)
        {
            renderContext->DestroyBuffer(meshInstanceIndicesBuffer);
        }

        if (meshInstanceIndicesBufferSrv)
        {
            renderContext->DestroyGpuView(meshInstanceIndicesBufferSrv);
        }

        if (gpuConstantsCbv)
        {
            renderContext->DestroyGpuView(gpuConstantsCbv);
        }

        if (gpuConstantsBuffer)
        {
            renderContext->DestroyBuffer(gpuConstantsBuffer);
        }
    }

    void SceneProxy::Replicate(tf::Subflow& replicationSubflow, const LocalFrameIndex localFrameIdx, const World& world)
    {
        IG_CHECK(taskExecutor != nullptr);
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        ZoneScopedN("SceneProxy.ReplicateScene");

        if (invalidationFuture[localFrameIdx].valid())
        {
            invalidationFuture[localFrameIdx].wait();
        }

        const Registry& registry = world.GetRegistry();

        tf::Task updateLightTask = replicationSubflow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.UpdateLightProxy");
                UpdateLightProxy(subflow, registry);
            }).name("SceneProxy.UpdateLightProxy");

        tf::Task updateMaterialTask = replicationSubflow.emplace(
            [this]()
            {
                // Material의 경우 분산 처리하기에는 상대적으로 그 수가 매우 작을 수 있기 때문에
                // 별도의 분산처리는 하지 않는다.
                ZoneScopedN("SceneProxy.UpdateMaterialProxy");

                // 현재 스레드의 로그를 잠시 막을 수 있지만, 만약 work stealing이 일어난다면
                // 다른 부분에서 발생하는 로그가 기록되지 않는 현상이 일어 날 수도 있음.
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateMaterialProxy();
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            }).name("SceneProxy.UpdateMaterialProxy");

        tf::Task updateStaticMeshTask = replicationSubflow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.UpdateStaticMeshProxy");
                Logger::GetInstance().SuppressLogInCurrentThread();
                UpdateStaticMeshProxy(subflow);
                Logger::GetInstance().UnsuppressLogInCurrentThread();
            }).name("SceneProxy.UpdateStaticMeshProxy");

        tf::Task updateSkeletalMeshTask = replicationSubflow.emplace(
            [this]([[maybe_unused]] tf::Subflow& subflow) {}).name("SceneProxy.UpdateSkeletalMeshProxy");

        tf::Task updateMeshInstanceTask = replicationSubflow.emplace(
            [this, &registry](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.UpdateMeshInstanceProxy");
                UpdateMeshInstanceProxy(subflow, registry);
            }).name("SceneProxy.UpdateMeshInstanceProxy");

        updateMeshInstanceTask.succeed(updateMaterialTask, updateStaticMeshTask, updateSkeletalMeshTask);

        tf::Task replicateLightData = replicationSubflow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.ReplicateLightData");
                ReplicateProxyData(subflow, localFrameIdx, lightProxyPackage);
            }).name("SceneProxy.ReplicateLightData");

        tf::Task replicateMaterialData = replicationSubflow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.ReplicateMaterialData");
                ReplicateProxyData(subflow, localFrameIdx, materialProxyPackage);
            }).name("SceneProxy.ReplicateMaterialData");

        tf::Task replicateStaticMeshData = replicationSubflow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.ReplicateStaticMeshData");
                ReplicateProxyData(subflow, localFrameIdx, staticMeshProxyPackage);
            }).name("SceneProxy.ReplicateStaticMeshData");

        tf::Task replicateMeshInstanceData = replicationSubflow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.ReplicateMeshInstanceData");
                ReplicateProxyData(subflow, localFrameIdx, meshInstanceProxyPackage);
            }).name("SceneProxy.ReplicateMeshInstanceData");

        tf::Task uploadMeshInstanceIndices = replicationSubflow.emplace(
            [this, localFrameIdx](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.UploadMeshInstanceIndices");
                UploadMeshInstanceIndices(subflow, localFrameIdx);
            }).name("SceneProxy.UploadMeshInstanceIndices");

        replicateLightData.succeed(updateLightTask);
        replicateMaterialData.succeed(updateMaterialTask);
        replicateStaticMeshData.succeed(updateStaticMeshTask);
        replicateMeshInstanceData.succeed(updateMeshInstanceTask);
        uploadMeshInstanceIndices.succeed(updateMeshInstanceTask);

        tf::Task updateGpuConstantsBuffer = replicationSubflow.emplace([this]()
        {
            ZoneScopedN("SceneProxy.UpdateGpuConstantsBuffer");
            bool bDirtyGpuConstants = false;
            const GpuView* lightStorageSrv = renderContext->Lookup(lightProxyPackage.Storage->GetSrv());
            IG_CHECK(lightStorageSrv != nullptr);
            if (gpuConstants.LightStorageSrv != lightStorageSrv->Index)
            {
                gpuConstants.LightStorageSrv = lightStorageSrv->Index;
                bDirtyGpuConstants = true;
            }
            const GpuView* materialStorageSrv = renderContext->Lookup(materialProxyPackage.Storage->GetSrv());
            IG_CHECK(materialStorageSrv != nullptr);
            if (gpuConstants.MaterialStorageSrv != materialStorageSrv->Index)
            {
                gpuConstants.MaterialStorageSrv = materialStorageSrv->Index;
                bDirtyGpuConstants = true;
            }
            const GpuView* staticMeshStorageSrv = renderContext->Lookup(staticMeshProxyPackage.Storage->GetSrv());
            IG_CHECK(staticMeshStorageSrv != nullptr);
            if (gpuConstants.StaticMeshStorageSrv != staticMeshStorageSrv->Index)
            {
                gpuConstants.StaticMeshStorageSrv = staticMeshStorageSrv->Index;
                bDirtyGpuConstants = true;
            }
            const GpuView* meshInstanceStorageSrv = renderContext->Lookup(meshInstanceProxyPackage.Storage->GetSrv());
            IG_CHECK(meshInstanceStorageSrv != nullptr);
            if (gpuConstants.MeshInstanceStorageSrv != meshInstanceStorageSrv->Index)
            {
                gpuConstants.MeshInstanceStorageSrv = meshInstanceStorageSrv->Index;
                bDirtyGpuConstants = true;
            }
            const GpuView* meshInstanceIndicesBufferSrvPtr = renderContext->Lookup(meshInstanceIndicesBufferSrv);
            IG_CHECK(meshInstanceIndicesBufferSrvPtr != nullptr);
            if (gpuConstants.MeshInstanceIndicesBufferSrv != meshInstanceIndicesBufferSrvPtr->Index)
            {
                gpuConstants.MeshInstanceIndicesBufferSrv = meshInstanceIndicesBufferSrvPtr->Index;
                bDirtyGpuConstants = true;
            }

            replicationSyncPoint = GpuSyncPoint::Invalid();
            if (bDirtyGpuConstants)
            {
                GpuBuffer* gpuConstantsBufferPtr = renderContext->Lookup(gpuConstantsBuffer);
                IG_CHECK(gpuConstantsBufferPtr != nullptr);
                GpuUploader& gpuUplaoder = renderContext->GetFrameCriticalGpuUploader();
                UploadContext uploadContext = gpuUplaoder.Reserve(sizeof(GpuConstants));
                GpuConstants* mappedBuffer = reinterpret_cast<GpuConstants*>(uploadContext.GetOffsettedCpuAddress());
                *mappedBuffer = gpuConstants;
                uploadContext.CopyBuffer(0, sizeof(GpuConstants), *gpuConstantsBufferPtr);
                replicationSyncPoint = gpuUplaoder.Submit(uploadContext);
            }
        }).name("SceneProxy.UpdateGpuConstants");
        updateGpuConstantsBuffer.succeed(replicateLightData, replicateMaterialData, replicateStaticMeshData, replicateMeshInstanceData, uploadMeshInstanceIndices);

        tf::Task finalizeReplication = replicationSubflow.emplace([this]()
        {
            if (!replicationSyncPoint.IsValid())
            {
                CommandQueue& asyncCopyQueue = renderContext->GetFrameCriticalAsyncCopyQueue();
                IG_CHECK(asyncCopyQueue.GetType() == EQueueType::Copy);
                replicationSyncPoint = asyncCopyQueue.MakeSyncPointWithSignal();
            }     
        }).name("SceneProxy.FinalizeReplication");
        finalizeReplication.succeed(updateGpuConstantsBuffer);
    }

    void SceneProxy::PrepareNextFrame(const LocalFrameIndex localFrameIdx)
    {
        // 다음 프레임 작업 준비
        const LocalFrameIndex nextLocalFrameIdx = (localFrameIdx + 1) % NumFramesInFlight;
        tf::Taskflow prepareNextFrameFlow{};
        [[maybe_unused]] tf::Task invalidateLightProxy = prepareNextFrameFlow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.InvalidateNextFrameLightProxy");
                subflow.for_each(
                    lightProxyPackage.ProxyMap.begin(), lightProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        [[maybe_unused]] tf::Task invalidateStaticMeshProxy = prepareNextFrameFlow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.InvalidateNextFrameStaticMeshProxy");
                subflow.for_each(
                    staticMeshProxyPackage.ProxyMap.begin(), staticMeshProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        [[maybe_unused]] tf::Task invalidateMaterialProxy = prepareNextFrameFlow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.InvalidateNextFrameMaterialProxy");
                subflow.for_each(
                    materialProxyPackage.ProxyMap.begin(), materialProxyPackage.ProxyMap.end(),
                    [](auto& keyValuePair)
                    {
                        keyValuePair.second.bMightBeDestroyed = true;
                    });
                subflow.join();
            });

        [[maybe_unused]] tf::Task invalidateMeshInstanceProxy = prepareNextFrameFlow.emplace(
            [this](tf::Subflow& subflow)
            {
                ZoneScopedN("SceneProxy.InvalidateNextFrameMeshInstProxy");
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
            [this, &entityProxyMap, lightView](const Entity entity)
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
                        proxy.GpuData.Forward = TransformUtility::MakeForward(transformComponent);

                        proxy.DataHashValue = currentDataHashValue;
                        lightProxyPackage.PendingReplicationGroups[workerId].emplace_back(entity);
                    }
                }
            }).name("SceneProxy.UpdateProxy");

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorkers; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : lightProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    lightProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            }).name("SceneProxy.CommitProxyConstructions");

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
            }).name("SceneProxy.CommitProxyDestructions");

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
                    /* 더 좋은 방식을 생각해야 할 필요성이 있음. 예시. Runtime 데이터를 따로 두고, Unload나 Save시 AssetManager 단에서 반영 */
                    std::optional<StaticMesh::LoadDesc> latestLoadDesc =
                        assetManager->GetLoadDesc<StaticMesh>(staticMeshPtr->GetSnapshot().Info.GetGuid());
                    IG_CHECK(latestLoadDesc.has_value());

                    if (const U64 currentDataHashValue = HashInstances(*staticMeshPtr, latestLoadDesc.value());
                        proxy.DataHashValue != currentDataHashValue)
                    {
                        const Mesh& mesh = staticMeshPtr->GetMesh();

                        const MeshVertexAllocation* vertexAllocPtr = unifiedMeshStorage.Lookup(mesh.VertexStorageAlloc);
                        IG_CHECK(vertexAllocPtr != nullptr);
                        proxy.GpuData.VertexStorageByteOffset = (U32)vertexAllocPtr->Alloc.Offset;
                        proxy.GpuData.NumLevelOfDetails = mesh.NumLevelOfDetails;
                        proxy.GpuData.bOverrideLodScreenCoverageThreshold = latestLoadDesc->bOverrideLodScreenCoverageThresholds;
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

                            proxy.GpuData.LodScreenCoverageThresholds[lod] = latestLoadDesc->LodScreenCoverageThresholds[lod];
                        }
                        proxy.GpuData.MeshBoundingSphere = ToBoundingSphere(mesh.BoundingBox);

                        proxy.DataHashValue = currentDataHashValue;
                        staticMeshProxyPackage.PendingReplicationGroups[workerId].emplace_back(cachedStaticMesh);
                    }
                }
            }).name("SceneProxy.UpdateProxy");

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &proxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorkers; ++groupIdx)
                {
                    for (auto& [pendingHandle, pendingProxy] : staticMeshProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        proxyMap[pendingHandle] = pendingProxy;
                    }
                    staticMeshProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            }).name("SceneProxy.CommitProxyConstructions");

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
            }).name("SceneProxy.CommitProxyDestructions");

        updateStaticMeshProxy.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);

        subflow.join();
    }

    void SceneProxy::UpdateMeshInstanceProxy(tf::Subflow& subflow, const Registry& registry)
    {
        IG_CHECK(meshInstanceProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = meshInstanceProxyPackage.ProxyMap;
        auto& storage = *meshInstanceProxyPackage.Storage;

        numMeshInstances = 0;
        for (Vector<U32>& meshInstanceIndices : meshInstanceIndicesGroups)
        {
            meshInstanceIndices.clear();
        }

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
                        const Matrix transformMat{TransformUtility::CreateTransformation(transform)};
                        proxy.GpuData.ToWorld[0] = Vector4{transformMat.m[0][0], transformMat.m[1][0], transformMat.m[2][0], transformMat.m[3][0]};
                        proxy.GpuData.ToWorld[1] = Vector4{transformMat.m[0][1], transformMat.m[1][1], transformMat.m[2][1], transformMat.m[3][1]};
                        proxy.GpuData.ToWorld[2] = Vector4{transformMat.m[0][2], transformMat.m[1][2], transformMat.m[2][2], transformMat.m[3][2]};

                        const MeshProxy& meshProxy = staticMeshProxyMap.at(staticMeshComponent.Mesh);
                        const MaterialProxy& materialProxy = materialProxyMap.at(materialComponent.Instance);

                        proxy.GpuData.MeshType = EMeshType::Static;
                        proxy.GpuData.MeshProxyIdx = (U32)meshProxy.StorageSpace.OffsetIndex;
                        proxy.GpuData.MaterialProxyIdx = (U32)materialProxy.StorageSpace.OffsetIndex;
                        pendingRepGroup.emplace_back(entity);
                    }

                    meshInstanceIndicesGroups[workerId].emplace_back((U32)proxy.StorageSpace.OffsetIndex);
                }
            }).name("SceneProxy.UpdateProxy");

        tf::Task commitPendingProxyTask = subflow.emplace(
            [this, &entityProxyMap, &storage]()
            {
                for (Index groupIdx = 0; groupIdx < numWorkers; ++groupIdx)
                {
                    for (auto& [pendingEntity, pendingProxy] : meshInstanceProxyPackage.PendingProxyGroups[groupIdx])
                    {
                        pendingProxy.StorageSpace = storage.Allocate(1);
                        entityProxyMap[pendingEntity] = pendingProxy;
                    }
                    meshInstanceProxyPackage.PendingProxyGroups[groupIdx].clear();
                }
            }).name("SceneProxy.CommitProxyConstructions");

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
            }).name("SceneProxy.CommitProxyDestructions");

        updateStaticMeshInstances.precede(commitPendingProxyTask);
        commitPendingProxyTask.precede(commitDestructions);

        subflow.join();
    }

    template <typename Proxy, typename Owner>
    void SceneProxy::ReplicateProxyData(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage)
    {
        IG_CHECK(proxyPackage.PendingReplicationGroups.size() == numWorkers);
        // 이미 그룹 별로 데이터가 균등 분배 되어있는 상황
        // 필요한 Staging Buffer 크기 == (sum(pendingRepsGroups[0..N].size()) * kDataSize))

        // #sy_todo 더 batching 잘 되도록 개선하기(한번 모은 다음에 재분배 하던가)
        tf::Task prepareReplication = subflow.emplace(
            [this, &proxyPackage, localFrameIdx]()
            {
                Size currentOffset = 0;
                for (Index groupIdx = 0; groupIdx < numWorkers; ++groupIdx)
                {
                    const Size workGroupDataSize = Proxy::kDataSize * proxyPackage.PendingReplicationGroups[groupIdx].size();
                    proxyPackage.WorkGroupStagingBufferRanges[groupIdx] = std::make_pair(currentOffset, workGroupDataSize);
                    currentOffset += workGroupDataSize;

                    CommandListPool& asyncCopyCmdListPool = renderContext->GetAsyncCopyCommandListPool();
                    if (workGroupDataSize > 0)
                    {
                        proxyPackage.WorkGroupCmdLists[groupIdx] = asyncCopyCmdListPool.Request(localFrameIdx, std::format("ProxyRep.{}", groupIdx));
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
            }).name("SceneProxy.ScheduleReplication");

        // SceneProxy가 Storage를 소유하고 있기때문에 작업 실행 중에 해제되지 않음이 보장됨.
        const Handle<GpuBuffer> storageBuffer = proxyPackage.Storage->GetGpuBuffer();
        GpuBuffer* storageBufferPtr = renderContext->Lookup(storageBuffer);
        IG_CHECK(storageBufferPtr != nullptr);
        tf::Task recordReplicateDataCmd = subflow.for_each_index(
            0, (S32)numWorkers, 1,
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
                            .GpuDataRef = CRef{proxy.GpuData}
                        });
                }
                IG_CHECK(!uploadInfos.empty());

                std::sort(
                    uploadInfos.begin(), uploadInfos.end(),
                    [](const typename Proxy::UploadInfo& lhs, const typename Proxy::UploadInfo& rhs)
                    {
                        return lhs.StorageSpaceRef.get().Offset < rhs.StorageSpaceRef.get().Offset;
                    });

                IG_CHECK(proxyPackage.StagingBuffer[localFrameIdx] != nullptr);
                IG_CHECK(storageBufferPtr != nullptr);
                IG_CHECK(proxyPackage.MappedStagingBuffer[localFrameIdx] != nullptr);

                const auto [stagingBufferOffset, allocatedSize] = proxyPackage.WorkGroupStagingBufferRanges[groupIdx];

                auto* mappedUploadBuffer = reinterpret_cast<typename Proxy::GpuData_t*>(
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

                    if (const bool bPendingNextBatching = bHasNextUploadInfo && bPendingCopyCommand;
                        bPendingNextBatching)
                    {
                        IG_CHECK((uploadInfos[idx].StorageSpaceRef.get().OffsetIndex + 1) <= uploadInfos[idx + 1].StorageSpaceRef.get().OffsetIndex);
                        srcOffset += copySize;
                        copySize = 0;
                        dstOffset = uploadInfos[idx + 1].StorageSpaceRef.get().Offset;
                    }
                }
                cmdList->Close();

                pendingReplications.clear();
            }).name("SceneProxy.RecordReplicationCommands");

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

                CommandQueue& cmdQueue = renderContext->GetFrameCriticalAsyncCopyQueue();
                IG_CHECK(cmdQueue.GetType() == EQueueType::Copy);
                cmdQueue.ExecuteCommandLists(std::span{
                    compactedCmdLists.data(),
                    compactedCmdLists.size()
                });
            }).name("SceneProxy.SubmitReplicationCommands");

        prepareReplication.precede(recordReplicateDataCmd);
        recordReplicateDataCmd.precede(submitCmdList);

        subflow.join();
    }

    void SceneProxy::ResizeMeshInstanceIndicesBuffer(const Size numMeshIndices)
    {
        IG_CHECK(numMeshIndices > 0);
        if (const Bytes requiredBufferSize = numMeshIndices * sizeof(U32);
            requiredBufferSize <= meshInstanceIndicesBufferSize)
        {
            return;
        }

        renderContext->DestroyBuffer(meshInstanceIndicesBuffer);
        renderContext->DestroyGpuView(meshInstanceIndicesBufferSrv);
        meshInstanceIndicesStagingBuffer.reset();

        /* 이 때, Staging Buffer는 2개 프레임 분의 버퍼를 재할당 하기 때문에 비효율 적일 수 있다. */
        const Size newNumMeshIndices = numMeshIndices + (numMeshIndices / 2);
        const Bytes newBufferSize = newNumMeshIndices * sizeof(U32);

        GpuBufferDesc meshInstanceIndicesBufferDesc;
        meshInstanceIndicesBufferDesc.DebugName = "MeshInstanceIndicesBuffer(SceneProxy)";
        meshInstanceIndicesBufferDesc.AsStructuredBuffer<U32>((U32)newNumMeshIndices);
        meshInstanceIndicesBuffer = renderContext->CreateBuffer(meshInstanceIndicesBufferDesc);
        meshInstanceIndicesBufferSrv = renderContext->CreateShaderResourceView(meshInstanceIndicesBuffer);

        meshInstanceIndicesStagingBuffer = MakePtr<GpuStagingBuffer>(
            *renderContext,
            GpuStagingBufferDesc{
                .BufferSize = newBufferSize,
                .DebugName = "MeshInstanceIndicesStagingBuffer(SceneProxy)"
            });

        meshInstanceIndicesBufferSize = newBufferSize;
    }

    void SceneProxy::UploadMeshInstanceIndices(tf::Subflow& subflow, const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        IG_CHECK(meshInstanceIndicesGroups.size() == numWorkers);
        IG_CHECK(meshInstanceIndicesUploadInfos.capacity() >= numWorkers);

        numMeshInstances = 0;
        meshInstanceIndicesUploadInfos.clear();
        U32 uploadOffsetBytes = 0; // 최종 UploadOffsetBytes == RequiredStagingBufferSize
        for (Index groupIdx = 0; groupIdx < numWorkers; ++groupIdx)
        {
            Vector<U32>& meshInstanceIndices = meshInstanceIndicesGroups[groupIdx];
            const U32 uploadSizeBytes = (U32)(meshInstanceIndices.size() * sizeof(U32));
            meshInstanceIndicesUploadInfos.emplace_back(uploadOffsetBytes, groupIdx);
            numMeshInstances += (U32)meshInstanceIndices.size();
            uploadOffsetBytes += uploadSizeBytes;
        }
        IG_CHECK(sizeof(U32)*numMeshInstances == uploadOffsetBytes);

        if (uploadOffsetBytes == 0)
        {
            return;
        }

        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshInstanceIndicesBuffer);
        IG_CHECK(meshInstanceIndicesStagingBuffer != nullptr);
        ResizeMeshInstanceIndicesBuffer(numMeshInstances);
        IG_CHECK(uploadOffsetBytes <= meshInstanceIndicesBufferSize);

        [[maybe_unused]] tf::Task writeMeshInstanceIndicesToStagingBuffer = subflow.for_each(
            meshInstanceIndicesUploadInfos.cbegin(), meshInstanceIndicesUploadInfos.cend(),
            [this, localFrameIdx](const std::pair<U32, Index>& uploadInfo)
            {
                IG_CHECK(uploadInfo.second < numWorkers);
                const Vector<U32>& meshInstanceIndices = meshInstanceIndicesGroups[uploadInfo.second];
                U8* mappedStagingBuffer = meshInstanceIndicesStagingBuffer->GetMappedBuffer(localFrameIdx);
                IG_CHECK(mappedStagingBuffer != nullptr);
                std::memcpy(mappedStagingBuffer + uploadInfo.first,
                    meshInstanceIndices.data(), meshInstanceIndices.size() * sizeof(U32));
            });
        subflow.join();

        GpuBuffer* bufferPtr = renderContext->Lookup(meshInstanceIndicesBuffer);
        IG_CHECK(bufferPtr != nullptr);
        GpuBuffer* stagingBufferPtr = renderContext->Lookup(meshInstanceIndicesStagingBuffer->GetBuffer(localFrameIdx));
        IG_CHECK(stagingBufferPtr != nullptr);
        CommandQueue& copyQueue = renderContext->GetFrameCriticalAsyncCopyQueue();
        IG_CHECK(copyQueue.GetType() == EQueueType::Copy);
        CommandListPool& copyCmdListPool = renderContext->GetAsyncCopyCommandListPool();
        auto copyCmdList = copyCmdListPool.Request(localFrameIdx, "UploadMeshInstanceIndices");
        copyCmdList->Open();
        copyCmdList->CopyBuffer(*stagingBufferPtr, 0, uploadOffsetBytes, *bufferPtr, 0);
        copyCmdList->Close();

        CommandList* cmdLists[]{copyCmdList};
        copyQueue.ExecuteCommandLists(cmdLists);
    }
} // namespace ig
