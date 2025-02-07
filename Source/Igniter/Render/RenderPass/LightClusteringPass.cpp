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
    struct LightClusteringConstants
    {
        U32 PerFrameDataCbv;
        U32 LightClusteringParamsCbv;
    };
} // namespace ig

namespace ig
{
    LightClusteringPass::LightClusteringPass(RenderContext& renderContext, const SceneProxy& sceneProxy, RootSignature& bindlessRootSignature, const Viewport& mainViewport)
        : renderContext(&renderContext)
        , sceneProxy(&sceneProxy)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        const ShaderCompileDesc clearU32BufferShaderDesc{.SourcePath = "Assets/Shaders/ClearU32Buffer.hlsl"_fs, .Type = EShaderType::Compute};
        clearU32BufferShader = MakePtr<ShaderBlob>(clearU32BufferShaderDesc);
        ComputePipelineStateDesc clearU32BufferPsoDesc{};
        clearU32BufferPsoDesc.Name = "ClearU32BufferPSO"_fs;
        clearU32BufferPsoDesc.SetComputeShader(*clearU32BufferShader);
        clearU32BufferPsoDesc.SetRootSignature(bindlessRootSignature);
        clearU32BufferPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(clearU32BufferPsoDesc).value());
        IG_CHECK(clearU32BufferPso->IsCompute());

        const ShaderCompileDesc lightClusteringShaderDesc{.SourcePath = "Assets/Shaders/LightClustering.hlsl"_fs, .Type = EShaderType::Compute};
        lightClusteringShader = MakePtr<ShaderBlob>(lightClusteringShaderDesc);
        ComputePipelineStateDesc lightClusteringPsoDesc{};
        lightClusteringPsoDesc.Name = "LightClusteringPSO"_fs;
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
            lightIdxListStagingBufferDesc.DebugName = String(std::format("LightIdxListStaging.{}", localFrameIdx));

            const RenderHandle<GpuBuffer> newLightIdxListStagingBuffer = renderContext.CreateBuffer(lightIdxListStagingBufferDesc);
            lightIdxListStagingBuffer[localFrameIdx] = newLightIdxListStagingBuffer;
            GpuBuffer* newLightIdxListStagingBufferPtr = renderContext.Lookup(newLightIdxListStagingBuffer);
            mappedLightIdxListStagingBuffer[localFrameIdx] = reinterpret_cast<U32*>(newLightIdxListStagingBufferPtr->Map());
        }

        GpuBufferDesc lightIdxListBufferDesc{};
        lightIdxListBufferDesc.AsStructuredBuffer<U32>(kMaxNumLights, false, false);
        lightIdxListBufferDesc.DebugName = String(std::format("LightIdxListBuffer"));
        lightIdxListBufferPackage.Buffer = renderContext.CreateBuffer(lightIdxListBufferDesc);
        lightIdxListBufferPackage.Srv = renderContext.CreateShaderResourceView(lightIdxListBufferPackage.Buffer);

        tileDwordsStride = kNumDWordsPerTile * numTileX;
        numTileDwords = tileDwordsStride * numTileY;
        GpuBufferDesc tileDwordsBufferDesc{};
        GpuBufferDesc depthBinsBufferDesc{};
        tileDwordsBufferDesc.AsStructuredBuffer<U32>((U32)numTileDwords, true);
        depthBinsBufferDesc.AsStructuredBuffer<DepthBin>((U32)kNumDepthBins, true);
        tileDwordsBufferDesc.DebugName = String(std::format("TileDwordsBuffer"));
        depthBinsBufferDesc.DebugName = String(std::format("DepthBinsBuffer"));

        const RenderHandle<GpuBuffer> newTileDwordsBuffer = renderContext.CreateBuffer(tileDwordsBufferDesc);
        tileDwordsBufferPackage.Buffer = newTileDwordsBuffer;
        tileDwordsBufferPackage.Srv = renderContext.CreateShaderResourceView(newTileDwordsBuffer);
        tileDwordsBufferPackage.Uav = renderContext.CreateUnorderedAccessView(newTileDwordsBuffer);

        const RenderHandle<GpuBuffer> newDepthBinsBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
        depthBinsBufferPackage.Buffer = newDepthBinsBuffer;
        depthBinsBufferPackage.Srv = renderContext.CreateShaderResourceView(newDepthBinsBuffer);
        depthBinsBufferPackage.Uav = renderContext.CreateUnorderedAccessView(newDepthBinsBuffer);

        depthBinsBufferDesc.DebugName = "DepthBinInitBuffer"_fs;
        depthBinInitBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
        GpuBuffer* depthBinInitBufferPtr = renderContext.Lookup(depthBinInitBuffer);
        GpuUploader& gpuUploader = renderContext.GetGpuUploader();
        UploadContext uploadCtx = gpuUploader.Reserve(depthBinsBufferDesc.GetSizeAsBytes());
        DepthBin* uploadDataPtr = reinterpret_cast<DepthBin*>(uploadCtx.GetOffsettedCpuAddress());
        for (Index idx = 0; idx < kNumDepthBins; ++idx)
        {
            uploadDataPtr[idx] = DepthBin{.FirstLightIdx = 0xFFFFFFFF, .LastLightIdx = 0};
        }
        uploadCtx.CopyBuffer(0, depthBinsBufferDesc.GetSizeAsBytes(), *depthBinInitBufferPtr);
        gpuUploader.Submit(uploadCtx)->WaitOnCpu();
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
        renderContext->DestroyGpuView(tileDwordsBufferPackage.Srv);
        renderContext->DestroyGpuView(tileDwordsBufferPackage.Uav);
        renderContext->DestroyBuffer(tileDwordsBufferPackage.Buffer);

        renderContext->DestroyGpuView(depthBinsBufferPackage.Srv);
        renderContext->DestroyGpuView(depthBinsBufferPackage.Uav);
        renderContext->DestroyBuffer(depthBinsBufferPackage.Buffer);
        renderContext->DestroyBuffer(depthBinInitBuffer);
    }

    void LightClusteringPass::SetParams(const LightClusteringPassParams& newParams)
    {
        IG_CHECK(newParams.InitCopyCmdList != nullptr);
        IG_CHECK(newParams.ClearTileDwordsBufferCmdList != nullptr);
        IG_CHECK(newParams.LightClusteringCmdList != nullptr);
        IG_CHECK(newParams.NearPlane < newParams.FarPlane);
        IG_CHECK(newParams.TargetViewport.width > 0.f && newParams.TargetViewport.height > 0.f);
        IG_CHECK(newParams.PerFrameCbvPtr != nullptr);
        IG_CHECK(newParams.GpuParamsConstantBuffer != nullptr);
        params = newParams;
    }

    void LightClusteringPass::OnExecute(const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(sceneProxy != nullptr);

        // Sorting Lights / Upload ight Idx List
        const auto& lightProxyMapValues = sceneProxy->GetLightProxyMap().values();
        const U32 numLights = (U32)std::min((Size)kMaxNumLights, lightProxyMapValues.size());
        lightProxyIdxViewZList.resize(numLights);

        tf::Executor& taskExecutor = Engine::GetTaskExecutor();
        tf::Taskflow buildLightIdxList{};

        tf::Task prepareIntermediateList = buildLightIdxList.for_each_index(
            0i32, (I32)numLights, 1i32,
            [this, &lightProxyMapValues](const Size idx)
            {
                const SceneProxy::LightProxy& proxy = lightProxyMapValues[idx].second;
                  lightProxyIdxViewZList[idx] = std::make_pair(
                      (U16)idx,
                    proxy.GpuData.WorldPosition.x * params.ViewMat._13 + proxy.GpuData.WorldPosition.y * params.ViewMat._23 + proxy.GpuData.WorldPosition.z * params.ViewMat._33);
            });

        tf::Task sortIntermediateList = buildLightIdxList.sort(
            lightProxyIdxViewZList.begin(), lightProxyIdxViewZList.end(),
            [](const std::pair<U16, float>& lhs, const std::pair<U16, float>& rhs)
            { return lhs.second < rhs.second; });

        tf::Task updateToStagingBuffer = buildLightIdxList.for_each_index(
            0i32, (I32)numLights, 1i32,
            [this, &lightProxyMapValues, localFrameIdx](const Size lightIdxListIdx)
            {
                const Size lightProxyIdx = lightProxyIdxViewZList[lightIdxListIdx].first;
                const SceneProxy::LightProxy& lightProxy = lightProxyMapValues[lightProxyIdx].second;
                mappedLightIdxListStagingBuffer[localFrameIdx][lightIdxListIdx] = (U32)lightProxy.StorageSpace.OffsetIndex;
            });

        prepareIntermediateList.precede(sortIntermediateList);
        sortIntermediateList.precede(updateToStagingBuffer);
        tf::Future<void> buildLightIdxListFut = taskExecutor.run(buildLightIdxList);

        {
            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            IG_CHECK(lightIdxListStagingBufferPtr != nullptr);
            GpuBuffer* lightIdxListBufferPtr = renderContext->Lookup(lightIdxListBufferPackage.Buffer);
            IG_CHECK(lightIdxListBufferPtr != nullptr);
            GpuBuffer* depthBinInitBufferPtr = renderContext->Lookup(depthBinInitBuffer);
            IG_CHECK(depthBinInitBufferPtr != nullptr);
            GpuBuffer* depthBinsBufferPtr = renderContext->Lookup(depthBinsBufferPackage.Buffer);
            IG_CHECK(depthBinsBufferPtr != nullptr);

            CommandList& lightIdxListCopyCmdList = *params.InitCopyCmdList;
            lightIdxListCopyCmdList.Open();
            lightIdxListCopyCmdList.CopyBuffer(*lightIdxListStagingBufferPtr, 0, sizeof(U32) * lightProxyIdxViewZList.size(), *lightIdxListBufferPtr, 0);
            lightIdxListCopyCmdList.CopyBuffer(*depthBinInitBufferPtr, *depthBinsBufferPtr);
            lightIdxListCopyCmdList.Close();
        }

        const GpuView* lightStorageBufferSrvPtr = renderContext->Lookup(sceneProxy->GetLightStorageSrv());
        IG_CHECK(lightStorageBufferSrvPtr != nullptr);
        const GpuView* lightIdxListBufferSrvPtr = renderContext->Lookup(lightIdxListBufferPackage.Srv);
        IG_CHECK(lightIdxListBufferSrvPtr != nullptr);
        const GpuView* tileDwordsBufferUavPtr = renderContext->Lookup(tileDwordsBufferPackage.Uav);
        IG_CHECK(tileDwordsBufferUavPtr != nullptr);
        const GpuView* depthBinsBufferUavPtr = renderContext->Lookup(depthBinsBufferPackage.Uav);
        IG_CHECK(depthBinsBufferUavPtr != nullptr);

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        const auto descriptorHeapsSpan = MakeSpan(descriptorHeaps);

        // Clear TileDwordsBuffer
        {
            CommandList& tileDwordsBufferClearCmdList = *params.ClearTileDwordsBufferCmdList;
            tileDwordsBufferClearCmdList.Open(clearU32BufferPso.get());

            tileDwordsBufferClearCmdList.SetDescriptorHeaps(descriptorHeapsSpan);
            tileDwordsBufferClearCmdList.SetRootSignature(*bindlessRootSignature);

            const U64 clearParams = (numTileDwords << 32) | tileDwordsBufferUavPtr->Index;
            tileDwordsBufferClearCmdList.SetRoot32BitConstants(0, clearParams, 0);

            constexpr U32 NumThreads = 1024;
            const U32 numThreadGroup = ((U32)numTileDwords + NumThreads - 1) / NumThreads;
            tileDwordsBufferClearCmdList.Dispatch(numThreadGroup, 1, 1);

            tileDwordsBufferClearCmdList.Close();
        }

        // Light Clustering
        {
            const LightClusteringPassGpuParams gpuParams{
                .ToProj = ConvertToShaderSuitableForm(params.ProjMat),
                .ViewportWidth = params.TargetViewport.width,
                .ViewportHeight = params.TargetViewport.height,
                .NumLights = (U32)numLights,
                .LightStorageBufferSrv = lightStorageBufferSrvPtr->Index,
                .LightIdxListBufferSrv = lightIdxListBufferSrvPtr->Index,
                .TileDwordsBufferUav = tileDwordsBufferUavPtr->Index,
                .DepthBinsBufferUav = depthBinsBufferUavPtr->Index};
            params.GpuParamsConstantBuffer->Write(gpuParams);

            const GpuView* gpuParamsCbvPtr = renderContext->Lookup(params.GpuParamsConstantBuffer->GetConstantBufferView());
            IG_CHECK(gpuParamsCbvPtr != nullptr);

            CommandList& lightClusteringCmdList = *params.LightClusteringCmdList;
            lightClusteringCmdList.Open(lightClusteringPso.get());

            lightClusteringCmdList.SetDescriptorHeaps(descriptorHeapsSpan);
            lightClusteringCmdList.SetRootSignature(*bindlessRootSignature);

            const LightClusteringConstants gpuConstants{
                .PerFrameDataCbv = params.PerFrameCbvPtr->Index,
                .LightClusteringParamsCbv = gpuParamsCbvPtr->Index};
            lightClusteringCmdList.SetRoot32BitConstants(0, gpuConstants, 0);

            constexpr U32 NumThreads = 32;
            const U32 numThreadGroup = (numLights + NumThreads - 1) / NumThreads;
            lightClusteringCmdList.Dispatch(numThreadGroup, 1, 1);

            lightClusteringCmdList.Close();
        }

        buildLightIdxListFut.wait();
    }
} // namespace ig
