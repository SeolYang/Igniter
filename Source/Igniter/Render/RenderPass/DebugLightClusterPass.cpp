#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/CommandSignature.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/DebugLightClusterPass.h"

namespace ig
{
    struct DebugLightClustersConstants
    {
        U32 PerFrameDataCbv;
        U32 ViewportWidth;
        U32 ViewportHeight;

        U32 NumLights;

        U32 TileDwordsBufferSrv;
        U32 InputTexSrv;
        U32 OutputTexUav;
    };
} // namespace ig

namespace ig
{
    DebugLightClusterPass::DebugLightClusterPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, const Viewport& mainViewport)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        const ShaderCompileDesc shaderDesc{.SourcePath = "Assets/Shaders/DebugLightCluster.hlsl"_fs, .Type = EShaderType::Compute};
        shader = MakePtr<ShaderBlob>(shaderDesc);
        ComputePipelineStateDesc psoDesc{};
        psoDesc.Name = "DebugLightClusterPassPSO"_fs;
        psoDesc.SetRootSignature(bindlessRootSignature);
        psoDesc.SetComputeShader(*shader);
        pso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(psoDesc).value());
        IG_CHECK(pso->IsCompute());

        constexpr DXGI_FORMAT kOutputTexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        GpuTextureDesc outputTexDesc{};
        outputTexDesc.AsTexture2D((U32)mainViewport.width, (U32)mainViewport.height,
            1, kOutputTexFormat, true);
        outputTexDesc.InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;

        outputTexDesc.DebugName = String(std::format("DebugLightClusterPassOutputTex"));

        outputTex = renderContext.CreateTexture(outputTexDesc);
        outputTexUav = renderContext.CreateUnorderedAccessView(outputTex,
            D3D12_TEX2D_UAV{.MipSlice = 0, .PlaneSlice = 0},
            kOutputTexFormat);
    }

    DebugLightClusterPass::~DebugLightClusterPass()
    {
        renderContext->DestroyGpuView(outputTexUav);
        renderContext->DestroyTexture(outputTex);
    }

    void DebugLightClusterPass::SetParams(const DebugLightClusterPassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.PerFrameDataCbv != nullptr);
        IG_CHECK(newParams.MainViewport.width > 0.f && newParams.MainViewport.height > 0.f);
        params = newParams;
    }

    void DebugLightClusterPass::OnRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);

        const GpuView* tileDwordsBufferSrvPtr = renderContext->Lookup(params.TileDwordsBufferSrv);
        IG_CHECK(tileDwordsBufferSrvPtr != nullptr);
        GpuTexture* inputTexPtr = renderContext->Lookup(params.InputTex);
        IG_CHECK(inputTexPtr != nullptr);
        const GpuView* inputTexSrvPtr = renderContext->Lookup(params.InputTexSrv);
        IG_CHECK(inputTexSrvPtr != nullptr);
        GpuTexture* outputTexPtr = renderContext->Lookup(outputTex);
        IG_CHECK(outputTexPtr != nullptr);
        const GpuView* outputTexUavPtr = renderContext->Lookup(outputTexUav);
        IG_CHECK(outputTexUavPtr != nullptr);

        CommandList& cmdList = *params.CmdList;
        cmdList.Open(pso.get());

        auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(bindlessDescHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        const DebugLightClustersConstants constants{
            .PerFrameDataCbv = params.PerFrameDataCbv->Index,
            .ViewportWidth = (U32)params.MainViewport.width,
            .ViewportHeight = (U32)params.MainViewport.height,
            .NumLights = params.NumLights,
            .TileDwordsBufferSrv = tileDwordsBufferSrvPtr->Index,
            .InputTexSrv = inputTexSrvPtr->Index,
            .OutputTexUav = outputTexUavPtr->Index
        };
        cmdList.SetRoot32BitConstants(0, constants, 0);

        cmdList.AddPendingTextureBarrier(
            *outputTexPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS);
        cmdList.FlushBarriers();

        constexpr U32 kNumThreadX = 8;
        constexpr U32 kNumThreadY = 8;
        const U32 dispatchX = (constants.ViewportWidth + kNumThreadX - 1) / kNumThreadX;
        const U32 dispatchY = (constants.ViewportHeight + kNumThreadY - 1) / kNumThreadY;
        cmdList.Dispatch(dispatchX, dispatchY, 1);

        cmdList.AddPendingTextureBarrier(
            *inputTexPtr,
            D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
            D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_ACCESS_NO_ACCESS,
            D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_COMMON);
        cmdList.FlushBarriers();

        cmdList.Close();
    }
} // namespace ig
