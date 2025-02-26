#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/MeshInstancePass.h"

namespace ig
{
    MeshInstancePass::MeshInstancePass(RenderContext& renderContext, RootSignature& bindlessRootSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        const ShaderCompileDesc meshInstancePassShaderDesc
        {
            .SourcePath = "Assets/Shaders/MeshInstancePass.hlsl"_fs,
            .Type = EShaderType::Compute,
        };
        cs = MakePtr<ShaderBlob>(meshInstancePassShaderDesc);
        ComputePipelineStateDesc meshInstancePassPsoDesc{};
        meshInstancePassPsoDesc.Name = "MeshInstancePassPSO"_fs;
        meshInstancePassPsoDesc.SetComputeShader(*cs);
        meshInstancePassPsoDesc.SetRootSignature(bindlessRootSignature);
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        pso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(meshInstancePassPsoDesc).value());
    }

    MeshInstancePass::~MeshInstancePass()
    {
        /* Empty Here! */
    }

    void MeshInstancePass::SetParams(const MeshInstancePassParams& newParams)
    {
        IG_CHECK(newParams.ComputeCmdList != nullptr);
        IG_CHECK(newParams.ZeroFilledBuffer != nullptr);
        IG_CHECK(newParams.PerFrameParamsCbv != nullptr);
        IG_CHECK(newParams.MeshInstanceIndicesBufferSrv != nullptr);
        IG_CHECK(newParams.DispatchOpaqueMeshInstanceStorageBuffer != nullptr);
        IG_CHECK(newParams.DispatchOpaqueMeshInstanceStorageUav != nullptr);
        IG_CHECK(newParams.DispatchTransparentMeshInstanceStorageBuffer == nullptr && "미구현");
        IG_CHECK(newParams.DispatchTransparentMeshInstanceStorageUav == nullptr && "미구현");
        IG_CHECK(newParams.OpaqueMeshInstanceIndicesStorageUav == nullptr && "미구현");
        IG_CHECK(newParams.TransparentMeshInstanceIndicesStorageUav == nullptr && "미구현");
        IG_CHECK(newParams.DepthPyramidParamsCbv != nullptr);
        params = newParams;
    }

    void MeshInstancePass::OnRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(cs != nullptr);
        IG_CHECK(pso != nullptr);

        const MeshInstancePassConstants meshInstancePassConstants
        {
            .PerFrameParamsCbv = params.PerFrameParamsCbv->Index,
            .MeshInstanceIndicesBufferSrv = params.MeshInstanceIndicesBufferSrv->Index,
            .NumMeshInstances = params.NumMeshInstances,
            .OpaqueMeshInstanceDispatchBufferUav = params.DispatchOpaqueMeshInstanceStorageUav->Index,
            .TransparentMeshInstanceDispatchBufferUav = 0xFFFFFFFF,
            .DepthPyramidParamsCbv = params.DepthPyramidParamsCbv->Index
        };

        CommandList& cmdList = *params.ComputeCmdList;
        cmdList.Open(pso.get());

        GpuBuffer* dispatchOpaqueMeshInstanceStorageBufferPtr = params.DispatchOpaqueMeshInstanceStorageBuffer;
        const GpuBufferDesc& opaqueMeshInstanceDispatchBufferDesc = dispatchOpaqueMeshInstanceStorageBufferPtr->GetDesc();
        cmdList.AddPendingBufferBarrier(
            *dispatchOpaqueMeshInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
        cmdList.FlushBarriers();

        cmdList.CopyBuffer(
            *params.ZeroFilledBuffer, 0, GpuBufferDesc::kUavCounterSize,
            *dispatchOpaqueMeshInstanceStorageBufferPtr, opaqueMeshInstanceDispatchBufferDesc.GetUavCounterOffset());

        cmdList.AddPendingBufferBarrier(
            *dispatchOpaqueMeshInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
        cmdList.FlushBarriers();

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);
        cmdList.SetRoot32BitConstants(0, meshInstancePassConstants, 0);

        if (params.NumMeshInstances > 0)
        {
            constexpr U32 kNumThreadsPerGroup = 32;
            const U32 numDispatchX = (meshInstancePassConstants.NumMeshInstances + kNumThreadsPerGroup - 1) / kNumThreadsPerGroup;
            cmdList.Dispatch(numDispatchX, 1, 1);
        }
        cmdList.Close();
    }
}
