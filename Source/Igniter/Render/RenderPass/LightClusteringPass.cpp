#include "Igniter/Igniter.h"
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

        GpuBufferDesc lightIdxListStagingBufferDesc{};
        lightIdxListStagingBufferDesc.AsUploadBuffer((U32)sizeof(U32) * kMaxNumLights);

        GpuBufferDesc lightIdxListBufferDesc{};
        lightIdxListBufferDesc.AsStructuredBuffer<U32>(kMaxNumLights, false, false);

        GpuBufferDesc tileDwordsBufferDesc{};
        GpuBufferDesc depthBinsBufferDesc{};

        numTileX = (Size)mainViewport.width / kTileSize;
        numTileY = (Size)mainViewport.height / kTileSize;

        tileDwordsStride = kNumDWordsPerTile * numTileX;
        numTileDwords = tileDwordsStride * numTileY;
        tileDwordsBufferDesc.AsStructuredBuffer<U32>((U32)numTileDwords, true);
        depthBinsBufferDesc.AsStructuredBuffer<DepthBin>((U32)kNumDepthBins, true);

        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            lightIdxListStagingBufferDesc.DebugName = String(std::format("LightIdxListStaging.{}", localFrameIdx));
            lightIdxListBufferDesc.DebugName = String(std::format("LightIdxListBuffer.{}", localFrameIdx));

            const RenderHandle<GpuBuffer> newLightIdxListStagingBuffer = renderContext.CreateBuffer(lightIdxListStagingBufferDesc);
            lightIdxListStagingBuffer[localFrameIdx] = newLightIdxListStagingBuffer;
            GpuBuffer* newLightIdxListStagingBufferPtr = renderContext.Lookup(newLightIdxListStagingBuffer);
            mappedLightIdxListStagingBuffer[localFrameIdx] = reinterpret_cast<U32*>(newLightIdxListStagingBufferPtr->Map());

            const RenderHandle<GpuBuffer> newLightIdxListBuffer = renderContext.CreateBuffer(lightIdxListBufferDesc);
            lightIdxListBufferPackage[localFrameIdx].Buffer = newLightIdxListBuffer;
            lightIdxListBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newLightIdxListBuffer);

            tileDwordsBufferDesc.DebugName = String(std::format("TileDwordsBuffer.{}", localFrameIdx));
            depthBinsBufferDesc.DebugName = String(std::format("DepthBinsBuffer.{}", localFrameIdx));

            const RenderHandle<GpuBuffer> newTileDwordsBuffer = renderContext.CreateBuffer(tileDwordsBufferDesc);
            tileDwordsBufferPackage[localFrameIdx].Buffer = newTileDwordsBuffer;
            tileDwordsBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newTileDwordsBuffer);
            tileDwordsBufferPackage[localFrameIdx].Uav = renderContext.CreateUnorderedAccessView(newTileDwordsBuffer);

            const RenderHandle<GpuBuffer> newDepthBinsBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
            depthBinsBufferPackage[localFrameIdx].Buffer = newDepthBinsBuffer;
            depthBinsBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newDepthBinsBuffer);
            depthBinsBufferPackage[localFrameIdx].Uav = renderContext.CreateUnorderedAccessView(newDepthBinsBuffer);
        }
    }

    LightClusteringPass::~LightClusteringPass()
    {
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            renderContext->DestroyGpuView(lightIdxListBufferPackage[localFrameIdx].Srv);
            renderContext->DestroyBuffer(lightIdxListBufferPackage[localFrameIdx].Buffer);
            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            lightIdxListStagingBufferPtr->Unmap();
            renderContext->DestroyBuffer(lightIdxListStagingBuffer[localFrameIdx]);

            renderContext->DestroyGpuView(tileDwordsBufferPackage[localFrameIdx].Srv);
            renderContext->DestroyGpuView(tileDwordsBufferPackage[localFrameIdx].Uav);
            renderContext->DestroyBuffer(tileDwordsBufferPackage[localFrameIdx].Buffer);

            renderContext->DestroyGpuView(depthBinsBufferPackage[localFrameIdx].Srv);
            renderContext->DestroyGpuView(depthBinsBufferPackage[localFrameIdx].Uav);
            renderContext->DestroyBuffer(depthBinsBufferPackage[localFrameIdx].Buffer);
        }
    }

    void LightClusteringPass::SetParams(const LightClusteringPassParams& newParams)
    {
        IG_CHECK(newParams.LightIdxListCopyCmdList != nullptr);
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

        const auto& lightProxyMapValues = sceneProxy->GetLightProxyMap(localFrameIdx).values();

        // Sorting Lights / Upload ight Idx List
        {
            lightProxyIdxViewZList.clear();
            IG_CHECK(lightProxyIdxViewZList.capacity() == kMaxNumLights);

            for (Index idx = 0; idx < std::min((Size)kMaxNumLights, lightProxyMapValues.size()); ++idx)
            {
                const SceneProxy::LightProxy& proxy = lightProxyMapValues[idx].second;
                lightProxyIdxViewZList.emplace_back((U16)idx, 
                                                    Vector4::Transform(Vector4(proxy.GpuData.WorldPosition.x, proxy.GpuData.WorldPosition.y, proxy.GpuData.WorldPosition.z, 1.f), params.ViewMat).z);
            }

            std::sort(lightProxyIdxViewZList.begin(), lightProxyIdxViewZList.end(),
                      [this, &lightProxyMapValues](const auto& lhs, const auto& rhs)
                      {
                          return lhs.second < rhs.second;
                      });

            for (Size lightIdxListIdx = 0; lightIdxListIdx < lightProxyIdxViewZList.size(); ++lightIdxListIdx)
            {
                const Size lightProxyIdx = lightProxyIdxViewZList[lightIdxListIdx].first;
                const SceneProxy::LightProxy& lightProxy = lightProxyMapValues[lightProxyIdx].second;
                mappedLightIdxListStagingBuffer[localFrameIdx][lightIdxListIdx] = (U32)lightProxy.StorageSpace.OffsetIndex;
            }

            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            IG_CHECK(lightIdxListStagingBufferPtr != nullptr);
            GpuBuffer* lightIdxListBufferPtr = renderContext->Lookup(lightIdxListBufferPackage[localFrameIdx].Buffer);
            IG_CHECK(lightIdxListBufferPtr != nullptr);

            CommandList& lightIdxListCopyCmdList = *params.LightIdxListCopyCmdList;
            lightIdxListCopyCmdList.Open();
            lightIdxListCopyCmdList.CopyBuffer(*lightIdxListStagingBufferPtr, 0, sizeof(U32) * lightProxyIdxViewZList.size(), *lightIdxListBufferPtr, 0);
            lightIdxListCopyCmdList.Close();
        }

        const GpuView* lightStorageBufferSrvPtr = renderContext->Lookup(sceneProxy->GetLightStorageSrv(localFrameIdx));
        IG_CHECK(lightStorageBufferSrvPtr != nullptr);
        const GpuView* lightIdxListBufferSrvPtr = renderContext->Lookup(lightIdxListBufferPackage[localFrameIdx].Srv);
        IG_CHECK(lightIdxListBufferSrvPtr != nullptr);
        const GpuView* tileDwordsBufferUavPtr = renderContext->Lookup(tileDwordsBufferPackage[localFrameIdx].Uav);
        IG_CHECK(tileDwordsBufferUavPtr != nullptr);
        const GpuView* depthBinsBufferUavPtr = renderContext->Lookup(depthBinsBufferPackage[localFrameIdx].Uav);
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
            const U32 numLights = (U32)lightProxyIdxViewZList.size();
            const LightClusteringPassGpuParams gpuParams{
                .ToProj = ConvertToShaderSuitableForm(params.ProjMat),
                .ViewportWidth = params.TargetViewport.width,
                .ViewportHeight = params.TargetViewport.height,
                .NumLights = numLights,
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
    }
} // namespace ig
