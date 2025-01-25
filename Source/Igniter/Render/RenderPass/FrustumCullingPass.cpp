#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/FrustumCullingPass.h"

/* Private Structures */
namespace ig
{
    struct FrustumCullingConstants
    {
        U32 PerFrameCbv;
        U32 MeshLodInstanceStorageUav;
        U32 CullingDataBufferUav;
    };

    struct StaticMeshLodInstance
    {
        U32 RenderableIdx = IG_NUMERIC_MAX_OF(RenderableIdx);
        U32 Lod = 0;
        U32 LodInstanceId = 0;
    };

    struct CullingData
    {
        U32 NumVisibleLodInstances;
    };
} // namespace ig

namespace ig
{
    FrustumCullingPass::FrustumCullingPass(RenderContext& renderContext, RootSignature& bindlessRootSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        const ShaderCompileDesc frustumCullingShaderDesc{.SourcePath = "Assets/Shaders/FrustumCulling.hlsl"_fs, .Type = EShaderType::Compute};
        shader = MakePtr<ShaderBlob>(frustumCullingShaderDesc);
        ComputePipelineStateDesc frustumCullingPsoDesc{};
        frustumCullingPsoDesc.Name = "FrustumCullingPSO"_fs;
        frustumCullingPsoDesc.SetComputeShader(*shader);
        frustumCullingPsoDesc.SetRootSignature(bindlessRootSignature);
        pso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(frustumCullingPsoDesc).value());
        IG_CHECK(pso->IsCompute());

        GpuBufferDesc cullingDataBufferDesc{};
        cullingDataBufferDesc.AsStructuredBuffer<CullingData>(1, true);
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            cullingDataBufferDesc.DebugName = std::format("CullingDataBuffer.{}", localFrameIdx);
            cullingDataBuffer[localFrameIdx] = renderContext.CreateBuffer(cullingDataBufferDesc);
            cullingDataBufferSrv[localFrameIdx] = renderContext.CreateShaderResourceView(cullingDataBuffer[localFrameIdx]);
            cullingDataBufferUav[localFrameIdx] = renderContext.CreateUnorderedAccessView(cullingDataBuffer[localFrameIdx]);
        }

        constexpr EGpuStorageFlags meshLodInstanceStorageFlags =
            EGpuStorageFlags::ShaderReadWrite |
            EGpuStorageFlags::EnableUavCounter |
            EGpuStorageFlags::EnableLinearAllocation;
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            meshLodInstanceStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                GpuStorageDesc{
                    .DebugName = String(std::format("MeshLodInstanceStorage.{}", localFrameIdx)),
                    .ElementSize = (U32)sizeof(StaticMeshLodInstance),
                    .NumInitElements = kNumInitMeshLodInstances,
                    .Flags = meshLodInstanceStorageFlags});
        }
    }

    FrustumCullingPass::~FrustumCullingPass()
    {
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            renderContext->DestroyGpuView(cullingDataBufferSrv[localFrameIdx]);
            renderContext->DestroyGpuView(cullingDataBufferUav[localFrameIdx]);
            renderContext->DestroyBuffer(cullingDataBuffer[localFrameIdx]);
        }
    }

    void FrustumCullingPass::SetParams(const FrustumCullingPassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.ZeroFilledBufferPtr != nullptr);
        IG_CHECK(newParams.PerFrameCbvPtr != nullptr);
        IG_CHECK(newParams.NumRenderables > 0);
        params = newParams;
    }

    void FrustumCullingPass::PreExecute(const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(meshLodInstanceStorage[localFrameIdx] != nullptr &&
                 meshLodInstanceStorage[localFrameIdx]->IsLinearAllocator());

        meshLodInstanceStorage[localFrameIdx]->Allocate(params.NumRenderables);
    }

    void FrustumCullingPass::OnExecute(const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(pso != nullptr);
        IG_CHECK(meshLodInstanceStorage[localFrameIdx]->GetNumAllocatedElements() == params.NumRenderables);

        GpuBuffer* meshLodInstanceStorageBufferPtr = renderContext->Lookup(meshLodInstanceStorage[localFrameIdx]->GetGpuBuffer());
        IG_CHECK(meshLodInstanceStorageBufferPtr != nullptr);
        const GpuBufferDesc& meshLodInstanceStorageBufferDesc = meshLodInstanceStorageBufferPtr->GetDesc();
        IG_CHECK(meshLodInstanceStorageBufferDesc.IsUavCounterEnabled());
        const GpuView* meshLodInstanceStorageUavPtr = renderContext->Lookup(meshLodInstanceStorage[localFrameIdx]->GetUnorderedResourceView());
        IG_CHECK(meshLodInstanceStorageUavPtr != nullptr);

        GpuBuffer* cullingDataBufferPtr = renderContext->Lookup(cullingDataBuffer[localFrameIdx]);
        IG_CHECK(cullingDataBufferPtr != nullptr);
        const GpuView* cullingDataBufferUavPtr = renderContext->Lookup(cullingDataBufferUav[localFrameIdx]);
        IG_CHECK(cullingDataBufferUavPtr != nullptr);

        const FrustumCullingConstants frustumCullingConstants{
            .PerFrameCbv = params.PerFrameCbvPtr->Index,
            .MeshLodInstanceStorageUav = meshLodInstanceStorageUavPtr->Index,
            .CullingDataBufferUav = cullingDataBufferUavPtr->Index};

        CommandList& cmdList = *params.CmdList;
        cmdList.Open(pso.get());

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        cmdList.AddPendingBufferBarrier(
            *params.ZeroFilledBufferPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_SOURCE);
        cmdList.AddPendingBufferBarrier(
            *meshLodInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
        cmdList.AddPendingBufferBarrier(
            *cullingDataBufferPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
        cmdList.FlushBarriers();

        cmdList.CopyBuffer(
            *params.ZeroFilledBufferPtr, 0, GpuBufferDesc::kUavCounterSize,
            *meshLodInstanceStorageBufferPtr, meshLodInstanceStorageBufferDesc.GetUavCounterOffset());
        cmdList.CopyBuffer(
            *params.ZeroFilledBufferPtr, 0, sizeof(CullingData),
            *cullingDataBufferPtr, 0);

        cmdList.AddPendingBufferBarrier(
            *meshLodInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
        cmdList.AddPendingBufferBarrier(
            *cullingDataBufferPtr,
            D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
        cmdList.FlushBarriers();

        cmdList.SetRoot32BitConstants(0, frustumCullingConstants, 0);
        const U32 numThreadGroup = (U32)(params.NumRenderables + kNumThreadsPerGroup - 1) / kNumThreadsPerGroup;
        cmdList.Dispatch(numThreadGroup, 1, 1);

        cmdList.Close();
    }
} // namespace ig
