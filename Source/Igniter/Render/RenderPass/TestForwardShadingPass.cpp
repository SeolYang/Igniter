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
#include "Igniter/Render/RenderPass/TestForwardShadingPass.h"

/* Private Structures */
namespace ig
{

} // namespace ig

namespace ig
{
    TestForwardShadingPass::TestForwardShadingPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, const Viewport& mainViewport)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();

        const ShaderCompileDesc vsDesc{
            .SourcePath = "Assets/Shaders/BasicVertexShader.hlsl"_fs,
            .Type = EShaderType::Vertex};

        const ShaderCompileDesc psDesc{
            .SourcePath = "Assets/Shaders/BasicPixelShader.hlsl"_fs,
            .Type = EShaderType::Pixel};

        vs = MakePtr<ShaderBlob>(vsDesc);
        ps = MakePtr<ShaderBlob>(psDesc);

        // Reverse-Z PSO
        GraphicsPipelineStateDesc psoDesc;
        psoDesc.Name = "FowardShadingPSO"_fs;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{CD3DX12_DEFAULT{}};
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        pso = MakePtr<PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());

        constexpr DXGI_FORMAT kOutputTexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        GpuTextureDesc outputTexDesc{};
        outputTexDesc.AsRenderTarget((U32)mainViewport.width, (U32)mainViewport.height,
                                     1, kOutputTexFormat);
        outputTexDesc.InitialLayout = D3D12_BARRIER_LAYOUT_COMMON;
        outputTexDesc.DebugName = String(std::format("ForwardShadingOutputTex"));
        outputTex = renderContext.CreateTexture(outputTexDesc);
        outputTexRtv = renderContext.CreateRenderTargetView(outputTex,
                                                            D3D12_TEX2D_RTV{.MipSlice = 0, .PlaneSlice = 0},
                                                            kOutputTexFormat);
        outputTexSrv = renderContext.CreateShaderResourceView(outputTex,
                                                              D3D12_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = 1, .PlaneSlice = 0});
    }

    TestForwardShadingPass::~TestForwardShadingPass()
    {
        renderContext->DestroyGpuView(outputTexRtv);
        renderContext->DestroyGpuView(outputTexSrv);
        renderContext->DestroyTexture(outputTex);
    }

    void TestForwardShadingPass::SetParams(const TestForwardShadingPassParams newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.DrawInstanceCmdSignature != nullptr);
        IG_CHECK(newParams.DrawInstanceCmdStorageBuffer);
        IG_CHECK(newParams.Dsv);
        IG_CHECK(newParams.MainViewport.width > 0.f && newParams.MainViewport.height > 0.f);
        params = newParams;
    }

    void TestForwardShadingPass::OnExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(pso != nullptr);

        GpuBuffer* drawInstanceCmdStorageBuffer = renderContext->Lookup(params.DrawInstanceCmdStorageBuffer);
        IG_CHECK(drawInstanceCmdStorageBuffer != nullptr);

        GpuTexture* outputTexPtr = renderContext->Lookup(outputTex);
        IG_CHECK(outputTexPtr != nullptr);
        const GpuView* outputTexRtvPtr = renderContext->Lookup(outputTexRtv);
        IG_CHECK(outputTexRtvPtr != nullptr);

        GpuTexture* depthTex = renderContext->Lookup(params.DepthTex);
        const GpuView* dsv = renderContext->Lookup(params.Dsv);
        IG_CHECK(depthTex != nullptr);
        IG_CHECK(dsv != nullptr);

        CommandList& cmdList = *params.CmdList;
        cmdList.Open(pso.get());

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        cmdList.AddPendingTextureBarrier(
            *outputTexPtr,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
            D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
        cmdList.AddPendingBufferBarrier(
            *drawInstanceCmdStorageBuffer,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT);
        cmdList.AddPendingTextureBarrier(
            *depthTex,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_DEPTH_STENCIL,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
            D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ);
        cmdList.FlushBarriers();

        cmdList.ClearRenderTarget(*outputTexRtvPtr);
        cmdList.SetRenderTarget(*outputTexRtvPtr, *dsv);
        cmdList.SetViewport(params.MainViewport);
        cmdList.SetScissorRect(params.MainViewport);
        cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList.ExecuteIndirect(*params.DrawInstanceCmdSignature, *drawInstanceCmdStorageBuffer);

        cmdList.AddPendingTextureBarrier(
            *outputTexPtr,
            D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
            D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
            D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
        cmdList.FlushBarriers();

        cmdList.Close();
    }
} // namespace ig
