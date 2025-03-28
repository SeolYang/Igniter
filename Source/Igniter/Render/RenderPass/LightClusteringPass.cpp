#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/Render/Light.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/CommandListPool.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Render/RenderPass/LightClusteringPass.h"

namespace ig
{
    LightClusteringPass::LightClusteringPass(RenderContext& renderContext, const SceneProxy& sceneProxy, RootSignature& bindlessRootSignature, const Viewport& mainViewport)
        : renderContext(&renderContext)
        , sceneProxy(&sceneProxy)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        constexpr ShaderCompileDesc clearU32BufferShaderDesc{
            .SourcePath = "Assets/Shaders/ClearU32Buffer.hlsl",
            .Type = EShaderType::Compute
        };
        clearU32BufferShader = MakePtr<ShaderBlob>(clearU32BufferShaderDesc);
        ComputePipelineStateDesc clearU32BufferPsoDesc{};
        clearU32BufferPsoDesc.Name = "ClearU32BufferPSO";
        clearU32BufferPsoDesc.SetComputeShader(*clearU32BufferShader);
        clearU32BufferPsoDesc.SetRootSignature(bindlessRootSignature);
        clearU32BufferPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(clearU32BufferPsoDesc).value());
        IG_CHECK(clearU32BufferPso->IsCompute());

        constexpr ShaderCompileDesc lightClusteringShaderDesc{
            .SourcePath = "Assets/Shaders/LightClustering.hlsl",
            .Type = EShaderType::Compute
        };
        lightClusteringShader = MakePtr<ShaderBlob>(lightClusteringShaderDesc);
        ComputePipelineStateDesc lightClusteringPsoDesc{};
        lightClusteringPsoDesc.Name = "LightClusteringPSO";
        lightClusteringPsoDesc.SetComputeShader(*lightClusteringShader);
        lightClusteringPsoDesc.SetRootSignature(bindlessRootSignature);
        lightClusteringPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(lightClusteringPsoDesc).value());
        IG_CHECK(lightClusteringPso->IsCompute());

        lightProxyIdxViewZList.reserve(kMaxNumLights);

        numTileX = (Size)mainViewport.width / kTileSize;
        numTileY = (Size)mainViewport.height / kTileSize;

        GpuBufferDesc lightIdxListStagingBufferDesc{};
        lightIdxListStagingBufferDesc.AsUploadBuffer((U32)sizeof(U32) * kMaxNumLights);
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            const std::string lightIdxListStagingBufferDebugName = std::format("LightIdxListStaging.{}", localFrameIdx);
            lightIdxListStagingBufferDesc.DebugName = lightIdxListStagingBufferDebugName;

            const Handle<GpuBuffer> newLightIdxListStagingBuffer = renderContext.CreateBuffer(lightIdxListStagingBufferDesc);
            lightIdxListStagingBuffer[localFrameIdx] = newLightIdxListStagingBuffer;
            GpuBuffer* newLightIdxListStagingBufferPtr = renderContext.Lookup(newLightIdxListStagingBuffer);
            mappedLightIdxListStagingBuffer[localFrameIdx] = reinterpret_cast<U32*>(newLightIdxListStagingBufferPtr->Map());
        }

        GpuBufferDesc lightIdxListBufferDesc{};
        lightIdxListBufferDesc.AsStructuredBuffer<U32>(kMaxNumLights, false, false);
        lightIdxListBufferDesc.DebugName = "LightIdxListBuffer";
        lightIdxListBufferPackage.Buffer = renderContext.CreateBuffer(lightIdxListBufferDesc);
        lightIdxListBufferPackage.Srv = renderContext.CreateShaderResourceView(lightIdxListBufferPackage.Buffer);

        tileDwordsStride = kNumDwordsPerTile * numTileX;
        numTileDwords = tileDwordsStride * numTileY;
        GpuBufferDesc lightTileBitfieldsBufferDesc{};
        GpuBufferDesc depthBinsBufferDesc{};
        lightTileBitfieldsBufferDesc.AsStructuredBuffer<U32>((U32)numTileDwords, true);
        depthBinsBufferDesc.AsStructuredBuffer<DepthBin>((U32)kNumDepthBins, true);
        lightTileBitfieldsBufferDesc.DebugName = "LightTileBitfields";
        depthBinsBufferDesc.DebugName = "DepthBins";

        const Handle<GpuBuffer> newTileDwordsBuffer = renderContext.CreateBuffer(lightTileBitfieldsBufferDesc);
        lightTileBitfieldsBufferPackage.Buffer = newTileDwordsBuffer;
        lightTileBitfieldsBufferPackage.Srv = renderContext.CreateShaderResourceView(newTileDwordsBuffer);
        lightTileBitfieldsBufferPackage.Uav = renderContext.CreateUnorderedAccessView(newTileDwordsBuffer);

        const Handle<GpuBuffer> newDepthBinsBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
        depthBinsBufferPackage.Buffer = newDepthBinsBuffer;
        depthBinsBufferPackage.Srv = renderContext.CreateShaderResourceView(newDepthBinsBuffer);
        depthBinsBufferPackage.Uav = renderContext.CreateUnorderedAccessView(newDepthBinsBuffer);

        depthBinsBufferDesc.DebugName = "DepthBinInitBuffer";
        depthBinInitBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
        GpuBuffer* depthBinInitBufferPtr = renderContext.Lookup(depthBinInitBuffer);
        GpuUploader& gpuUploader = renderContext.GetNonFrameCriticalGpuUploader();
        UploadContext depthBinInitContext = gpuUploader.Reserve(depthBinsBufferDesc.GetSizeAsBytes());
        DepthBin* uploadDataPtr = reinterpret_cast<DepthBin*>(depthBinInitContext.GetOffsettedCpuAddress());
        for (Index idx = 0; idx < kNumDepthBins; ++idx)
        {
            uploadDataPtr[idx] = DepthBin{.FirstLightIdx = 0xFFFFFFFF, .LastLightIdx = 0};
        }
        depthBinInitContext.CopyBuffer(0, depthBinsBufferDesc.GetSizeAsBytes(), *depthBinInitBufferPtr);
        gpuUploader.Submit(depthBinInitContext).WaitOnCpu();

        GpuBufferDesc lightClusterConstantsBufferDesc{};
        lightClusterConstantsBufferDesc.AsConstantBuffer<LightClusterConstants>();
        lightClusterConstantsBufferDesc.DebugName = "UnifiedMeshStorageConstants";
        lightClusterConstantsBufferDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        lightClusterConstantsBuffer = renderContext.CreateBuffer(lightClusterConstantsBufferDesc);
        lightClusterConstantsCbv = renderContext.CreateConstantBufferView(lightClusterConstantsBuffer);

        GpuBuffer* lightClusterConstantsBufferPtr = renderContext.Lookup(lightClusterConstantsBuffer);
        IG_CHECK(lightClusterConstantsBufferPtr != nullptr);
        UploadContext lightClusterConstantsUploadContext = gpuUploader.Reserve(sizeof(LightClusterConstants));
        LightClusterConstants* mappedConstants = reinterpret_cast<LightClusterConstants*>(lightClusterConstantsUploadContext.GetOffsettedCpuAddress());
        *mappedConstants = LightClusterConstants{
            .LightIdxListSrv = renderContext.Lookup(lightIdxListBufferPackage.Srv)->Index,
            .TileBitfieldsSrv = renderContext.Lookup(lightTileBitfieldsBufferPackage.Srv)->Index,
            .DepthBinsSrv = renderContext.Lookup(depthBinsBufferPackage.Srv)->Index
        };
        lightClusterConstantsUploadContext.CopyBuffer(0, sizeof(LightClusterConstants), *lightClusterConstantsBufferPtr);
        gpuUploader.Submit(lightClusterConstantsUploadContext).WaitOnCpu();
    }

    LightClusteringPass::~LightClusteringPass()
    {
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            lightIdxListStagingBufferPtr->Unmap();
            renderContext->DestroyBuffer(lightIdxListStagingBuffer[localFrameIdx]);
        }

        renderContext->DestroyGpuView(lightIdxListBufferPackage.Srv);
        renderContext->DestroyBuffer(lightIdxListBufferPackage.Buffer);
        renderContext->DestroyGpuView(lightTileBitfieldsBufferPackage.Srv);
        renderContext->DestroyGpuView(lightTileBitfieldsBufferPackage.Uav);
        renderContext->DestroyBuffer(lightTileBitfieldsBufferPackage.Buffer);

        renderContext->DestroyGpuView(depthBinsBufferPackage.Srv);
        renderContext->DestroyGpuView(depthBinsBufferPackage.Uav);
        renderContext->DestroyBuffer(depthBinsBufferPackage.Buffer);
        renderContext->DestroyBuffer(depthBinInitBuffer);

        renderContext->DestroyGpuView(lightClusterConstantsCbv);
        renderContext->DestroyBuffer(lightClusterConstantsBuffer);
    }

    void LightClusteringPass::SetParams(const LightClusteringPassParams& newParams)
    {
        IG_CHECK(newParams.CopyLightIdxListCmdList != nullptr);
        IG_CHECK(newParams.ClearTileBitfieldsCmdList != nullptr);
        IG_CHECK(newParams.LightClusteringCmdList != nullptr);
        IG_CHECK(newParams.TargetViewport.width > 0.f && newParams.TargetViewport.height > 0.f);
        IG_CHECK(newParams.PerFrameParamsCbvPtr != nullptr);
        IG_CHECK(newParams.SceneProxyConstantsCbvPtr != nullptr);
        params = newParams;
    }

    void LightClusteringPass::OnRecord(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(sceneProxy != nullptr);

        // Sorting Lights / Upload ight Idx List
        const auto& lightProxyMapValues = sceneProxy->GetLightProxyMap().values();
        const U32 numLights = (U32)std::min((Size)kMaxNumLights, lightProxyMapValues.size());
        lightProxyIdxViewZList.resize(numLights);

        tf::Executor& taskExecutor = Engine::GetTaskExecutor();
        tf::Taskflow buildLightIdxList{};

        tf::Task prepareIntermediateList = buildLightIdxList.for_each_index(
            0i32, (S32)numLights, 1i32,
            [this, &lightProxyMapValues](const Size idx)
            {
                const SceneProxy::LightProxy& proxy = lightProxyMapValues[idx].second;
                lightProxyIdxViewZList[idx] = std::make_pair(
                    (U16)idx,
                    proxy.GpuData.WorldPosition.x * params.View._13 + proxy.GpuData.WorldPosition.y * params.View._23 + proxy.GpuData.WorldPosition.z * params.View._33);
            });

        tf::Task sortIntermediateList = buildLightIdxList.sort(
            lightProxyIdxViewZList.begin(), lightProxyIdxViewZList.end(),
            [](const std::pair<U16, float>& lhs, const std::pair<U16, float>& rhs)
            {
                return lhs.second < rhs.second;
            });

        tf::Task updateToStagingBuffer = buildLightIdxList.for_each_index(
            0i32, (S32)numLights, 1i32,
            [this, &lightProxyMapValues, localFrameIdx](const Size lightIdxListIdx)
            {
                const Size lightProxyIdx = lightProxyIdxViewZList[lightIdxListIdx].first;
                const SceneProxy::LightProxy& lightProxy = lightProxyMapValues[lightProxyIdx].second;
                mappedLightIdxListStagingBuffer[localFrameIdx][lightIdxListIdx] = (U32)lightProxy.StorageSpace.OffsetIndex;
            });

        prepareIntermediateList.precede(sortIntermediateList);
        sortIntermediateList.precede(updateToStagingBuffer);
        tf::Future<void> buildLightIdxListFut = taskExecutor.run(buildLightIdxList);

        /* Light Idx List Copy (FrameCriticalCopyQueue) */
        {
            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            IG_CHECK(lightIdxListStagingBufferPtr != nullptr);
            GpuBuffer* lightIdxListBufferPtr = renderContext->Lookup(lightIdxListBufferPackage.Buffer);
            IG_CHECK(lightIdxListBufferPtr != nullptr);
            GpuBuffer* depthBinInitBufferPtr = renderContext->Lookup(depthBinInitBuffer);
            IG_CHECK(depthBinInitBufferPtr != nullptr);
            GpuBuffer* depthBinsBufferPtr = renderContext->Lookup(depthBinsBufferPackage.Buffer);
            IG_CHECK(depthBinsBufferPtr != nullptr);

            CommandList& lightIdxListCopyCmdList = *params.CopyLightIdxListCmdList;
            lightIdxListCopyCmdList.Open();
            if (!lightProxyIdxViewZList.empty())
            {
                lightIdxListCopyCmdList.CopyBuffer(*lightIdxListStagingBufferPtr, 0, sizeof(U32) * lightProxyIdxViewZList.size(), *lightIdxListBufferPtr, 0);
                lightIdxListCopyCmdList.CopyBuffer(*depthBinInitBufferPtr, *depthBinsBufferPtr);
            }
            lightIdxListCopyCmdList.Close();
        }

        const GpuView* lghtIdxListSrvPtr = renderContext->Lookup(lightIdxListBufferPackage.Srv);
        IG_CHECK(lghtIdxListSrvPtr != nullptr);
        const GpuView* tileBitfieldsUavPtr = renderContext->Lookup(lightTileBitfieldsBufferPackage.Uav);
        IG_CHECK(tileBitfieldsUavPtr != nullptr);
        const GpuView* depthBinsUavPtr = renderContext->Lookup(depthBinsBufferPackage.Uav);
        IG_CHECK(depthBinsUavPtr != nullptr);

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        const auto descriptorHeapsSpan = MakeSpan(descriptorHeaps);

        /* Clear TileBitfields (Compute Queue) */
        {
            CommandList& clearTileBitfieldsCmdList = *params.ClearTileBitfieldsCmdList;
            clearTileBitfieldsCmdList.Open(clearU32BufferPso.get());

            clearTileBitfieldsCmdList.SetDescriptorHeaps(descriptorHeapsSpan);
            clearTileBitfieldsCmdList.SetRootSignature(*bindlessRootSignature);

            const U64 clearParams = (numTileDwords << 32) | tileBitfieldsUavPtr->Index;
            clearTileBitfieldsCmdList.SetRoot32BitConstants(0, clearParams, 0);

            constexpr U32 kNumThreads = 1024;
            const U32 numThreadGroup = ((U32)numTileDwords + kNumThreads - 1) / kNumThreads;
            clearTileBitfieldsCmdList.Dispatch(numThreadGroup, 1, 1);

            clearTileBitfieldsCmdList.Close();
        }

        /* Light Clustering (Compute Queue) */
        {
            CommandList& lightClusteringCmdList = *params.LightClusteringCmdList;
            lightClusteringCmdList.Open(lightClusteringPso.get());

            if (numLights > 0)
            {
                lightClusteringCmdList.SetDescriptorHeaps(descriptorHeapsSpan);
                lightClusteringCmdList.SetRootSignature(*bindlessRootSignature);

                const LightClusteringPassConstants gpuConstants{
                    .PerFrameParamsCbv = params.PerFrameParamsCbvPtr->Index,
                    .SceneProxyConstantsCbv = params.SceneProxyConstantsCbvPtr->Index,
                    .LightIdxListSrv = lghtIdxListSrvPtr->Index,
                    .TileBitfieldsUav = tileBitfieldsUavPtr->Index,
                    .DepthBinsUav = depthBinsUavPtr->Index,
                    .NumLights = numLights
                };
                lightClusteringCmdList.SetRoot32BitConstants(0, gpuConstants, 0);

                constexpr U32 kNumThreads = 32;
                const U32 numThreadGroup = (numLights + kNumThreads - 1) / kNumThreads;
                lightClusteringCmdList.Dispatch(numThreadGroup, 1, 1);
            }

            lightClusteringCmdList.Close();
        }

        buildLightIdxListFut.wait();
    }
} // namespace ig
