#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
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
    SceneProxy::SceneProxy(RenderContext& renderContext, const MeshStorage& meshStorage, const AssetManager& assetManager) :
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
            renderableProxyPackage.Storage[localFrameIdx]->ForceReset();
            lightProxyPackage.Storage[localFrameIdx]->ForceReset();

            renderContext->DestroyGpuView(renderableIndicesBufferSrv[localFrameIdx]);
            renderContext->DestroyGpuView(lightIndicesBufferSrv[localFrameIdx]);

            renderContext->DestroyBuffer(renderableIndicesBuffer[localFrameIdx]);
            renderContext->DestroyBuffer(lightIndicesBuffer[localFrameIdx]);
        }
    }

    void SceneProxy::Replicate(const LocalFrameIndex localFrameIdx, const World& world)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(assetManager != nullptr);
        const Registry& registry = world.GetRegistry();

        UpdateTransformProxy(localFrameIdx, registry);
        ReplicatePrxoyData(localFrameIdx, transformProxyPackage);

        UpdateMaterialProxy(localFrameIdx, registry);
        ReplicatePrxoyData(localFrameIdx, materialProxyPackage);

        UpdateRenderableProxy(localFrameIdx, registry);
        ReplicatePrxoyData(localFrameIdx, renderableProxyPackage);

        // 현재 프레임에서 존재 할 수 있는 Renderable/Light의 수를 카운트(numCurrentRenderables/numCurrentLights) 해서
        // 만약 각 인덱스 버퍼의 최대 수용 가능 인덱스 수(numMaxRenderables/numMaxLights)를 초과하면, GpuBuffer의
        // 크기를 재조정 해야한다. (ex. max(numMaxRenderables * 2, numCurrentRenderables))
        UpdateRenderableIndicesBuffer(localFrameIdx);
    }

    void SceneProxy::UpdateTransformProxy(const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        IG_CHECK(transformProxyPackage.PendingReplications.empty());
        IG_CHECK(transformProxyPackage.PendingDestructions.empty());

        auto& entityProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *transformProxyPackage.Storage[localFrameIdx];

        for (auto& [entity, proxy] : entityProxyMap)
        {
            proxy.bMightBeDestroyed = true;
        }

        auto staticMeshEntityView = registry.view<const TransformComponent, const StaticMeshComponent>();
        for (const auto& [entity, transformComponent, staticMeshComponent] : staticMeshEntityView.each())
        {
            if (!entityProxyMap.contains(entity))
            {
                entityProxyMap[entity] = TransformProxy{.StorageSpace = storage.Allocate(1)};
            }

            TransformProxy& transformProxy = entityProxyMap.at(entity);
            transformProxy.bMightBeDestroyed = false;

            if (const U32 currentDataHashValue = HashInstance(transformComponent);
                transformProxy.DataHashValue != currentDataHashValue)
            {
                transformProxy.GpuData = transformComponent.CreateTransformation();
                transformProxy.DataHashValue = currentDataHashValue;
                transformProxyPackage.PendingReplications.emplace_back(entity);
            }
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

    void SceneProxy::UpdateMaterialProxy(const LocalFrameIndex localFrameIdx, const Registry& registry)
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

        const auto materialProcedure = [](const RenderContext* renderContext,
                                          const AssetManager* assetManager,
                                          ProxyPackage<MaterialProxy, ManagedAsset<Material>>& proxyPackage,
                                          const LocalFrameIndex localFrameIdx,
                                          const ManagedAsset<Material> material)
        {
            auto& proxyMap = proxyPackage.ProxyMap[localFrameIdx];
            auto& storage = *proxyPackage.Storage[localFrameIdx];

            if (!material)
            {
                return;
            }

            if (!proxyMap.contains(material))
            {
                proxyMap[material] = MaterialProxy{.StorageSpace = storage.Allocate(1)};
            }

            MaterialProxy& proxy = proxyMap[material];
            proxy.bMightBeDestroyed = false;

            const Material* materialPtr = assetManager->Lookup(material);
            IG_CHECK(materialPtr != nullptr);
            if (const U32 currentDataHashValue = HashInstance(*materialPtr);
                proxy.DataHashValue != currentDataHashValue)
            {
                MaterialGpuData newData{};

                const Texture* diffuseTexturePtr = assetManager->Lookup(materialPtr->GetDiffuse());
                if (diffuseTexturePtr != nullptr)
                {
                    const GpuView* srvPtr = renderContext->Lookup(diffuseTexturePtr->GetShaderResourceView());
                    if (srvPtr != nullptr)
                    {
                        newData.DiffuseTextureShaderResourceViewSlot = srvPtr->Index;
                    }
                }

                proxy.GpuData = newData;
                proxy.DataHashValue = currentDataHashValue;
                proxyPackage.PendingReplications.emplace_back(material);
            }
        };

        // TODO 가능한 모든 종류의 renderable object(material을 소유한)로 부터 material 핸들 수집
        auto staticMeshEntityView = registry.view<const StaticMeshComponent>();
        for (const auto& [entity, staticMeshComponent] : staticMeshEntityView.each())
        {
            if (!staticMeshComponent.Mesh)
            {
                continue;
            }

            const StaticMesh* staticMeshPtr = assetManager->Lookup(staticMeshComponent.Mesh);
            IG_CHECK(staticMeshPtr != nullptr);
            materialProcedure(renderContext, assetManager,
                              materialProxyPackage, localFrameIdx,
                              staticMeshPtr->GetMaterial());
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

    void SceneProxy::UpdateRenderableProxy(const LocalFrameIndex localFrameIdx, const Registry& registry)
    {
        IG_CHECK(renderableProxyPackage.PendingReplications.empty());
        IG_CHECK(renderableProxyPackage.PendingDestructions.empty());

        auto& proxyMap = renderableProxyPackage.ProxyMap[localFrameIdx];
        auto& storage = *renderableProxyPackage.Storage[localFrameIdx];
        const auto& transformProxyMap = transformProxyPackage.ProxyMap[localFrameIdx];
        const auto& materialProxyMap = materialProxyPackage.ProxyMap[localFrameIdx];

        for (auto& [entity, proxy] : proxyMap)
        {
            IG_CHECK(entity != NullEntity);
            proxy.bMightBeDestroyed = true;
        }

        // TODO 가능한 모든 종류의 renderable object(material을 소유한)로 부터 데이터 수집
        renderableIndices.clear();
        auto staticMeshEntityView = registry.view<const TransformComponent, const StaticMeshComponent>();
        for (const auto& [entity, transformComponent, staticMeshComponent] : staticMeshEntityView.each())
        {
            if (!staticMeshComponent.Mesh)
            {
                continue;
            }

            const StaticMesh* staticMeshPtr = assetManager->Lookup(staticMeshComponent.Mesh);
            IG_CHECK(staticMeshPtr != nullptr);
            IG_CHECK(transformProxyMap.contains(entity));
            IG_CHECK(staticMeshPtr->GetMaterial());

            if (!proxyMap.contains(entity))
            {
                proxyMap[entity] = RenderableProxy{.StorageSpace = storage.Allocate(1)};
            }

            RenderableProxy& proxy = proxyMap[entity];
            proxy.bMightBeDestroyed = false;
            renderableIndices.emplace_back((U32)proxy.StorageSpace.OffsetIndex);

            if (const U32 currentDataHashValue = HashInstance(*staticMeshPtr);
                proxy.DataHashValue != currentDataHashValue)
            {
                const MeshStorage::Handle<VertexSM> vertexSpace = staticMeshPtr->GetVertexSpace();
                const MeshStorage::Space<VertexSM>* vertexSpacePtr = meshStorage->Lookup(vertexSpace);
                const MeshStorage::Handle<U32> vertexIndexSpace = staticMeshPtr->GetVertexIndexSpace();
                const MeshStorage::Space<U32>* vertexIndexSpacePtr = meshStorage->Lookup(vertexIndexSpace);

                const RenderableGpuData newData{
                    .TransformStorageIndex = (U32)transformProxyMap.at(entity).StorageSpace.OffsetIndex,
                    .MaterialStorageIndex = (U32)materialProxyMap.at(staticMeshPtr->GetMaterial()).StorageSpace.OffsetIndex,
                    .VertexOffset = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.OffsetIndex : 0),
                    .NumVertices = (U32)(vertexSpacePtr != nullptr ? vertexSpacePtr->Allocation.NumElements : 0),
                    .IndexOffset = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.OffsetIndex : 0),
                    .NumIndices = (U32)(vertexIndexSpacePtr != nullptr ? vertexIndexSpacePtr->Allocation.NumElements : 0)};

                proxy.GpuData = newData;
                proxy.DataHashValue = currentDataHashValue;
                renderableProxyPackage.PendingReplications.emplace_back(entity);
            }
        }

        for (auto& [entity, proxy] : proxyMap)
        {
            if (proxy.bMightBeDestroyed)
            {
                renderableProxyPackage.PendingDestructions.emplace_back(entity);
            }
        }

        for (const Entity entity : renderableProxyPackage.PendingDestructions)
        {
            const auto extractedElement = proxyMap.extract(entity);
            IG_CHECK(extractedElement.has_value());
            IG_CHECK(extractedElement->second.StorageSpace.IsValid());
            storage.Deallocate(extractedElement->second.StorageSpace);
        }
        renderableProxyPackage.PendingDestructions.clear();
    }

    template <typename Proxy, typename Owner>
    void SceneProxy::ReplicatePrxoyData(const LocalFrameIndex localFrameIdx, ProxyPackage<Proxy, Owner>& proxyPackage)
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

            // <2>.
            std::sort(uploadInfos.begin(), uploadInfos.end(),
                      [](const Proxy::UploadInfo& lhs, const Proxy::UploadInfo& rhs)
                      {
                          return lhs.StorageSpaceRef.get().Offset < rhs.StorageSpaceRef.get().Offset;
                      });

            // <3>.
            GpuUploader& uploader = renderContext->GetGpuUploader();
            const Size requiredUploadBufferSize = Proxy::kDataSize * uploadInfos.size();
            IG_CHECK(requiredUploadBufferSize > 0);
            UploadContext uploadContext = uploader.Reserve(requiredUploadBufferSize);

            // <4 ~ 5>.
            const RenderHandle<GpuBuffer> storageBuffer = proxyPackage.Storage[localFrameIdx]->GetGpuBuffer();
            IG_CHECK(storageBuffer);
            GpuBuffer* storageBufferPtr = renderContext->Lookup(storageBuffer);
            IG_CHECK(storageBufferPtr != nullptr);

            auto* mappedUploadBuffer =
                reinterpret_cast<Proxy::GpuData_t*>(uploadContext.GetOffsettedCpuAddress());

            IG_CHECK(!uploadInfos.empty());
            Size srcOffset = 0;
            Size copySize = 0;
            Size dstOffset = uploadInfos[0].StorageSpaceRef.get().Offset;
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
                    uploadContext.CopyBuffer(srcOffset, copySize, *storageBufferPtr, dstOffset);
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
            proxyPackage.PendingReplications.clear();
            uploader.Submit(uploadContext)->WaitOnCpu();
        }
    }

    void SceneProxy::UpdateRenderableIndicesBuffer(const LocalFrameIndex localFrameIdx)
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

            GpuBufferDesc renderableIndicesBufferDesc;
            renderableIndicesBufferDesc.AsStructuredBuffer<U32>(numMaxRenderables[localFrameIdx]);
            renderableIndicesBufferDesc.DebugName = String(std::format("RenderableIndicesBuffer{}", localFrameIdx));
            renderableIndicesBuffer[localFrameIdx] = renderContext->CreateBuffer(renderableIndicesBufferDesc);
            renderableIndicesBufferSrv[localFrameIdx] = renderContext->CreateShaderResourceView(renderableIndicesBuffer[localFrameIdx]);

            renderableIndicesBufferSize[localFrameIdx] = numMaxRenderables[localFrameIdx];
        }

        IG_CHECK(renderableIndicesBuffer[localFrameIdx]);
        GpuBuffer* renderableIndicesBufferPtr = renderContext->Lookup(renderableIndicesBuffer[localFrameIdx]);
        IG_CHECK(renderableIndicesBufferPtr != nullptr);

        const Size copySize = sizeof(U32) * numMaxRenderables[localFrameIdx];
        GpuUploader& gpuUploader = renderContext->GetGpuUploader();
        UploadContext uploadContext = gpuUploader.Reserve(copySize);
        auto* mappedBuffer = reinterpret_cast<U32*>(uploadContext.GetOffsettedCpuAddress());
        IG_CHECK(mappedBuffer != nullptr);
        std::memcpy(mappedBuffer, renderableIndices.data(), copySize);

        uploadContext.CopyBuffer(0, copySize, *renderableIndicesBufferPtr);
        gpuUploader.Submit(uploadContext)->WaitOnCpu();
    }
} // namespace ig
