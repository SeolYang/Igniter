#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/CommandSignature.h"
#include "Igniter/D3D12/CommandSignatureDesc.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Asset/StaticMesh.h"
#include "Igniter/Render/RenderPass/GenerateMeshLodDrawCommandsPass.h"

/* Private Structures */
namespace ig
{
    struct DrawInstance
    {
        U32 PerFrameCbv;
        U32 MaterialIdx;
        U32 VertexOffset;
        U32 VertexIdxOffset;
        U32 IndirectTransformOffset;

        D3D12_DRAW_ARGUMENTS DrawIndexedArguments;
    };

    struct GenerateDrawInstanceConstants
    {
        U32 PerFrameCbv;
        U32 CullingDataBufferSrv;
        U32 DrawInstanceBufferUav;
    };
} // namespace ig

namespace ig
{
    GenerateMeshLodDrawCommandsPass::GenerateMeshLodDrawCommandsPass(RenderContext& renderContext, RootSignature& bindlessRootSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        const ShaderCompileDesc genDrawInstanceCmdShaderDesc{.SourcePath = "Assets/Shaders/GenerateDrawInstanceCommand.hlsl"_fs, .Type = EShaderType::Compute};
        shader = MakePtr<ShaderBlob>(genDrawInstanceCmdShaderDesc);
        IG_CHECK(shader->GetType() == EShaderType::Compute);

        ComputePipelineStateDesc generateDrawInstanceCmdPsoDesc{};
        generateDrawInstanceCmdPsoDesc.SetComputeShader(*shader);
        generateDrawInstanceCmdPsoDesc.SetRootSignature(bindlessRootSignature);
        generateDrawInstanceCmdPsoDesc.Name = "GenerateDrawInstanceCmdPso"_fs;
        pso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(generateDrawInstanceCmdPsoDesc).value());
        IG_CHECK(pso->IsCompute());

        drawInstanceCmdStorage = MakePtr<GpuStorage>(
            renderContext,
            GpuStorageDesc{
                .DebugName = String(std::format("DrawRenderableCmdBuffer")),
                .ElementSize = (U32)sizeof(DrawInstance),
                .NumInitElements = kNumInitDrawCommands,
                .Flags =
                    EGpuStorageFlags::ShaderReadWrite |
                    EGpuStorageFlags::EnableUavCounter |
                    EGpuStorageFlags::EnableLinearAllocation});

        CommandSignatureDesc commandSignatureDesc{};
        commandSignatureDesc.AddConstant(0, 0, 5)
            .AddDrawArgument()
            .SetCommandByteStride<DrawInstance>();
        cmdSignature = MakePtr<CommandSignature>(
            gpuDevice.CreateCommandSignature(
                         "DrawInstanceCmdSignature",
                         commandSignatureDesc,
                         bindlessRootSignature)
                .value());
    }

    GenerateMeshLodDrawCommandsPass::~GenerateMeshLodDrawCommandsPass()
    {
        /* Empty! */
    }

    void GenerateMeshLodDrawCommandsPass::SetParams(const GenerateMeshLodDrawCommandsPassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.PerFrameCbvPtr != nullptr);
        IG_CHECK(newParams.CullingDataBufferSrv);
        IG_CHECK(newParams.ZeroFilledBufferPtr != nullptr);
        IG_CHECK(newParams.NumInstancing > 0);
        params = newParams;
    }

    void GenerateMeshLodDrawCommandsPass::PreExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(drawInstanceCmdStorage != nullptr &&
                 drawInstanceCmdStorage->IsLinearAllocator());

        [[maybe_unused]] const auto linearAlloc = drawInstanceCmdStorage->Allocate((Size)params.NumInstancing * StaticMesh::kMaxNumLods);
    }

    void GenerateMeshLodDrawCommandsPass::OnExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(pso != nullptr);
        IG_CHECK(drawInstanceCmdStorage->GetNumAllocatedElements() == ((Size)params.NumInstancing * StaticMesh::kMaxNumLods));

        const GpuView* cullingDataBufferSrvPtr = renderContext->Lookup(params.CullingDataBufferSrv);
        IG_CHECK(cullingDataBufferSrvPtr != nullptr);

        GpuBuffer* drawInstanceStorageBufferPtr = renderContext->Lookup(drawInstanceCmdStorage->GetGpuBuffer());
        IG_CHECK(drawInstanceStorageBufferPtr != nullptr);
        const GpuBufferDesc& drawInstanceStorageBufferDesc = drawInstanceStorageBufferPtr->GetDesc();
        IG_CHECK(drawInstanceStorageBufferDesc.IsUavCounterEnabled());
        const GpuView* drawInstanceStorageUavPtr = renderContext->Lookup(drawInstanceCmdStorage->GetUnorderedResourceView());
        IG_CHECK(drawInstanceStorageUavPtr != nullptr);

        const GenerateDrawInstanceConstants generateInstanceDrawConstants{
            .PerFrameCbv = params.PerFrameCbvPtr->Index,
            .CullingDataBufferSrv = cullingDataBufferSrvPtr->Index,
            .DrawInstanceBufferUav = drawInstanceStorageUavPtr->Index};

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
            *drawInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COPY,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_COPY_DEST);
        cmdList.FlushBarriers();

        cmdList.CopyBuffer(
            *params.ZeroFilledBufferPtr, 0, GpuBufferDesc::kUavCounterSize,
            *drawInstanceStorageBufferPtr, drawInstanceStorageBufferDesc.GetUavCounterOffset());

        cmdList.AddPendingBufferBarrier(
            *drawInstanceStorageBufferPtr,
            D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
        cmdList.FlushBarriers();

        cmdList.SetRoot32BitConstants(0, generateInstanceDrawConstants, 0);
        const U32 numLodInstancing = params.NumInstancing * StaticMesh::kMaxNumLods;
        cmdList.Dispatch(numLodInstancing, 1, 1);

        cmdList.Close();
    }
} // namespace ig
