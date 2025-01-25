#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/CompactMeshLodInstancesPass.h"

/* Private Structures */
namespace ig
{
    struct CompactMeshLodInstancesConstants
    {
        U32 PerFrameCbv;
        U32 MeshLodInstanceStorageUav;
        U32 CullingDataBufferSrv;
    };

} // namespace ig

namespace ig
{
    CompactMeshLodInstancesPass::CompactMeshLodInstancesPass(RenderContext& renderContext, RootSignature& bindlessRootSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        const ShaderCompileDesc compactMeshLodInstancesDesc{.SourcePath = "Assets/Shaders/CompactMeshLodInstances.hlsl"_fs, .Type = EShaderType::Compute};
        shader = MakePtr<ShaderBlob>(compactMeshLodInstancesDesc);
        IG_CHECK(shader->GetType() == EShaderType::Compute);

        ComputePipelineStateDesc compactMeshLodInstancesPsoDesc{};
        compactMeshLodInstancesPsoDesc.Name = "CompactMeshLodInstancesPSO"_fs;
        compactMeshLodInstancesPsoDesc.SetComputeShader(*shader);
        compactMeshLodInstancesPsoDesc.SetRootSignature(bindlessRootSignature);
        pso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(compactMeshLodInstancesPsoDesc).value());
        IG_CHECK(compactMeshLodInstancesPso->IsCompute());
    }

    CompactMeshLodInstancesPass::~CompactMeshLodInstancesPass()
    {
        /* Empty! */
    }

    void CompactMeshLodInstancesPass::SetParams(const CompactMeshLodInstancesPassParams newParams)
    {
        IG_CHECK(params.CmdList != nullptr);
        IG_CHECK(params.PerFrameCbvPtr != nullptr);
        IG_CHECK(params.MeshLodInstanceStorageUavHandle);
        IG_CHECK(params.CullingDataBufferSrvHandle);
        IG_CHECK(params.NumRenderables > 0);
        params = newParams;
    }

    void CompactMeshLodInstancesPass::OnExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(pso != nullptr);

        const GpuView* meshLodInstanceStorageUavPtr = renderContext->Lookup(params.MeshLodInstanceStorageUavHandle);
        IG_CHECK(meshLodInstanceStorageUavPtr != nullptr);

        const GpuView* cullingDataBufferSrvPtr = renderContext->Lookup(params.CullingDataBufferSrvHandle);
        IG_CHECK(cullingDataBufferSrvPtr != nullptr);

        const CompactMeshLodInstancesConstants comapctMeshLodInstancesConstants{
            .PerFrameCbv = params.PerFrameCbvPtr->Index,
            .MeshLodInstanceStorageUav = meshLodInstanceStorageUavPtr->Index,
            .CullingDataBufferSrv = cullingDataBufferSrvPtr->Index};

        CommandList& cmdList = *params.CmdList;
        cmdList.Open(pso.get());

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        cmdList.SetRoot32BitConstants(0, comapctMeshLodInstancesConstants, 0);

        const U32 numThreadGroup = (U32)(params.NumRenderables + kNumThreadsPerGroup - 1) / kNumThreadsPerGroup;
        cmdList.Dispatch(numThreadGroup, 1, 1);

        cmdList.Close();
    }
} // namespace ig
