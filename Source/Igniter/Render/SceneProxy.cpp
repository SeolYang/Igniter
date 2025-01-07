#include "Igniter/Igniter.h"
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

            transformProxyPackage.PendingRepsPerThread.resize(std::thread::hardware_concurrency());
            transformProxyPackage.PendingDestsPerThread.resize(std::thread::hardware_concurrency());

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
        ZoneScoped;

        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        const Registry& registry = world.GetRegistry();

        Logger::GetInstance().SuppressLog();
        std::future<void> updateMaterialFuture = std::async(
            std::launch::async, [this, localFrameIdx, &registry]
            { UpdateMaterialProxy(localFrameIdx); });

        // #sy_todo Taskflow를 사용한 작업 흐름 구성?
        std::future<void> updateStaticMeshFuture = std::async(
            std::launch::async,
            [this, localFrameIdx, &registry, &updateMaterialFuture]
            {
                updateMaterialFuture.wait();
                UpdateStaticMeshProxy(localFrameIdx, registry);
            });

        UpdateTransformProxy(localFrameIdx, registry);
        updateStaticMeshFuture.get();
        Logger::GetInstance().UnsuppressLog();

        UpdateRenderableProxy(localFrameIdx);

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

    void SceneProxy::UpdateTransformProxy(const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        ZoneScoped;
        IG_CHECK(transformProxyPackage.PendingReplications.empty());
        IG_CHECK(transformProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *transformProxyPackage.Storage[localFrameIdx];

        std::for_each(std::execution::par_unseq,
                      entityProxyMap.begin(), entityProxyMap.end(),
                      [](auto& keyValuePair)
                      {
                          keyValuePair.second.bMightBeDestroyed = true;
                      });

        const auto transformView = registry.view<const TransformComponent>();
        for (const auto& [entity, transform] : transformView.each())
        {
            if (!entityProxyMap.contains(entity))
            {
                entityProxyMap[entity] = TransformProxy{.StorageSpace = storage.Allocate(1)};
            }
        }

        const U32 numConcurrentThreads = std::thread::hardware_concurrency();
        const Size numWorksPerThread = transformView.size() / numConcurrentThreads;
        const Size numRemainedWorks = transformView.size() % numConcurrentThreads;

        Vector<std::future<void>> threadFutures{};
        threadFutures.reserve(numConcurrentThreads);

        Size workElementOffset = 0;
        Vector<std::pair<Size, Size>> threadWorkRange{};
        threadWorkRange.reserve(numConcurrentThreads);

        for (U32 threadIdx = 0; threadIdx < numConcurrentThreads; ++threadIdx)
        {
            const Size numWorkElements = numWorksPerThread + (threadIdx == 0 ? numRemainedWorks : 0);
            threadWorkRange.emplace_back(workElementOffset, numWorkElements);
            workElementOffset += numWorkElements;
        }
        IG_CHECK(workElementOffset == transformView.size());

        for (U32 threadIdx = 0; threadIdx < numConcurrentThreads; ++threadIdx)
        {
            std::future<void> threadFuture = std::async(
                std::launch::async,
                [this, localFrameIdx, &transformView, &registry](const U32 threadIdx, const Size workElementOffset, const Size numWorkElements)
                {
                    ZoneScopedN("UpdateTransformPerThread");
                    for (Size elementIdx = 0; elementIdx < numWorkElements; ++elementIdx)
                    {
                        auto& entityProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];

                        const Entity entity = *(transformView.begin() + workElementOffset + elementIdx);
                        TransformProxy& proxy = entityProxyMap[entity];
                        IG_CHECK(proxy.bMightBeDestroyed);
                        proxy.bMightBeDestroyed = false;

                        const TransformComponent& transformComponent = registry.get<TransformComponent>(entity);
                        if (const U64 currentDataHashValue = HashInstance(transformComponent);
                            proxy.DataHashValue != currentDataHashValue)
                        {
                            proxy.GpuData = transformComponent.CreateTransformation();
                            proxy.DataHashValue = currentDataHashValue;

                            transformProxyPackage.PendingRepsPerThread[threadIdx].emplace_back(entity);
                        }
                    }
                },
                threadIdx,
                threadWorkRange[threadIdx].first, threadWorkRange[threadIdx].second);

            threadFutures.emplace_back(std::move(threadFuture));
        }

        for (std::future<void>& threadFuture : threadFutures)
        {
            threadFuture.get();
        }

        for (U32 threadIdx = 0; threadIdx < numConcurrentThreads; ++threadIdx)
        {
            for (const Entity entity : transformProxyPackage.PendingRepsPerThread[threadIdx])
            {
                transformProxyPackage.PendingReplications.emplace_back(entity);
            }
            transformProxyPackage.PendingRepsPerThread[threadIdx].clear();
        }

        // Proxy Map이 element deletion에 stable 하지 않기 때문에 별도로 처리
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
    }

    void SceneProxy::UpdateMaterialProxy(const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(materialProxyPackage.PendingReplications.empty());
        IG_CHECK(materialProxyPackage.PendingDestructions.empty());

        auto& proxyMap = materialProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *materialProxyPackage.Storage[localFrameIdx];

        for (auto& [material, proxy] : proxyMap)
        {
            IG_CHECK(material);
            proxy.bMightBeDestroyed = true;
        }

        std::vector<AssetManager::Snapshot> snapshots{assetManager->TakeSnapshots()};
        for (const AssetManager::Snapshot& snapshot : snapshots)
        {
            if (snapshot.Info.GetCategory() != EAssetCategory::Material)
            {
                continue;
            }

            if (!snapshot.IsCached())
            {
                continue;
            }

            ManagedAsset<Material> cachedMaterial = assetManager->LoadMaterial(snapshot.Info.GetGuid());
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

            assetManager->Unload(cachedMaterial);
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

    void SceneProxy::UpdateStaticMeshProxy(const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        ZoneScoped;
        IG_CHECK(staticMeshProxyPackage.PendingReplications.empty());
        IG_CHECK(staticMeshProxyPackage.PendingDestructions.empty());

        auto& proxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *staticMeshProxyPackage.Storage[localFrameIdx];
        const auto& materialProxyMap = materialProxyPackage.ProxyMap[localFrameIdx];

        for (auto& [entity, proxy] : proxyMap)
        {
            IG_CHECK(entity != NullEntity);
            proxy.bMightBeDestroyed = true;
        }

        auto staticMeshEntityView = registry.view<const StaticMeshComponent>();
        for (const auto& [entity, staticMeshComponent] : staticMeshEntityView.each())
        {
            if (!staticMeshComponent.Mesh)
            {
                continue;
            }

            if (!proxyMap.contains(entity))
            {
                proxyMap[entity] = StaticMeshProxy{.StorageSpace = storage.Allocate(1)};
            }

            StaticMeshProxy& proxy = proxyMap[entity];
            IG_CHECK(proxy.bMightBeDestroyed);
            proxy.bMightBeDestroyed = false;

            // Static Mesh Component의 경우, 에셋에 대한 핸들을 저장하고 핸들은 바뀌지 않더라도
            // Reimport/Reload로 인해서 실제 참조 할 데이터는 변경 될 수 있기 때문에
            const StaticMesh* staticMeshPtr = assetManager->Lookup(staticMeshComponent.Mesh);
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

                staticMeshProxyPackage.PendingReplications.emplace_back(entity);
            }
        }

        for (auto& [entity, proxy] : proxyMap)
        {
            if (proxy.bMightBeDestroyed)
            {
                staticMeshProxyPackage.PendingDestructions.emplace_back(entity);
            }
        }

        for (const Entity entity : staticMeshProxyPackage.PendingDestructions)
        {
            const auto extractedElement = proxyMap.extract(entity);
            IG_CHECK(extractedElement.has_value());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        staticMeshProxyPackage.PendingDestructions.clear();
    }

    void SceneProxy::UpdateRenderableProxy(const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderableProxyPackage.PendingReplications.empty());
        IG_CHECK(renderableProxyPackage.PendingDestructions.empty());

        auto& renderableProxyMap = renderableProxyPackage.ProxyMap[localFrameIdx];
        auto& renderableStorage = *renderableProxyPackage.Storage[localFrameIdx];
        const auto& staticMeshProxyMap = staticMeshProxyPackage.ProxyMap[localFrameIdx];
        const auto& transformProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];

        for (auto& [entity, proxy] : renderableProxyMap)
        {
            IG_CHECK(entity != NullEntity);
            proxy.bMightBeDestroyed = true;
        }

        renderableIndices.clear();

        // 만약 Renderable Component를 한 개의 Entity가 여러개 가지고 있더라도
        // renderable로 반영 되는 건 딱 첫 번째로 인식 되는 컴포넌트다.
        // TODO 이런 경우 처리를 따로 해야할까?
        // 가능한 방안은 같은 타입의 Renderable이 아니라면 bMightBeDestroyed를 false로 변경하지 않도록 한다.
        // 그렇게 하면 가장 먼저 업로드된 정보가 Expired 되어도 정상적으로 해당 Renderable에 대한 Destroy 알고리즘이
        // 작동하게 되고, 다음 프레임 부터는 정상적으로 새로운 Renderable이 할당되게 된다.
        for (const auto& [entity, _] : staticMeshProxyMap)
        {
            IG_CHECK(transformProxyMap.contains(entity));

            if (!renderableProxyMap.contains(entity))
            {
                renderableProxyMap[entity] = RenderableProxy{
                    .StorageSpace = renderableStorage.Allocate(1),
                    .GpuData = RenderableGpuData{
                        .Type = ERenderableType::StaticMesh,
                        .DataIdx = (U32)staticMeshProxyMap.at(entity).StorageSpace.OffsetIndex,
                        .TransformDataIdx = (U32)transformProxyMap.at(entity).StorageSpace.OffsetIndex}};

                renderableProxyPackage.PendingReplications.emplace_back(entity);
            }

            RenderableProxy& proxy = renderableProxyMap[entity];
            if (proxy.GpuData.Type != ERenderableType::StaticMesh)
            {
                continue;
            }

            IG_CHECK(proxy.bMightBeDestroyed);
            proxy.bMightBeDestroyed = false;
            renderableIndices.emplace_back((U32)proxy.StorageSpace.OffsetIndex);
        }

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
