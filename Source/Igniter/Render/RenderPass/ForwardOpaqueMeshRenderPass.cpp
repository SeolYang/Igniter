#include "Igniter/Igniter.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/ForwardOpaqueMeshRenderPass.h"

namespace ig
{
    ForwardOpaqueMeshRenderPass::ForwardOpaqueMeshRenderPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, CommandSignature& dispatchMeshInstanceCmdSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
        , dispatchMeshInstanceCmdSignature(&dispatchMeshInstanceCmdSignature)
    {
        ShaderCompileDesc forwardMeshInstanceAmpShaderDesc
        {
            .SourcePath = "Assets/Shaders/MeshInstanceAS.hlsl",
            .Type = EShaderType::Amplification
        };
        as = MakePtr<ShaderBlob>(forwardMeshInstanceAmpShaderDesc);

        ShaderCompileDesc forwardMeshInstanceMeshShaderDesc
        {
            .SourcePath = "Assets/Shaders/MeshInstanceMS.hlsl",
            .Type = EShaderType::Mesh
        };
        ms = MakePtr<ShaderBlob>(forwardMeshInstanceMeshShaderDesc);

        ShaderCompileDesc forwardMeshInstancePixelShaderDesc
        {
            .SourcePath = "Assets/Shaders/ForwardPS.hlsl",
            .Type = EShaderType::Pixel
        };
        ps = MakePtr<ShaderBlob>(forwardMeshInstancePixelShaderDesc);

        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        MeshShaderPipelineStateDesc forwardMeshInstancePipelineDesc{};
        forwardMeshInstancePipelineDesc.Name = "ForwardMeshInstancePSO";
        forwardMeshInstancePipelineDesc.SetRootSignature(bindlessRootSignature);
        forwardMeshInstancePipelineDesc.NumRenderTargets = 1;
        forwardMeshInstancePipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        forwardMeshInstancePipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        forwardMeshInstancePipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{CD3DX12_DEFAULT{}};
        forwardMeshInstancePipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        forwardMeshInstancePipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        forwardMeshInstancePipelineDesc.SetAmplificationShader(*as);
        forwardMeshInstancePipelineDesc.SetMeshShader(*ms);
        forwardMeshInstancePipelineDesc.SetPixelShader(*ps);
        pso = MakePtr<PipelineState>(gpuDevice.CreateMeshShaderPipelineState(forwardMeshInstancePipelineDesc).value());
    }

    ForwardOpaqueMeshRenderPass::~ForwardOpaqueMeshRenderPass()
    {
        /* Empty Here! */
    }

    void ForwardOpaqueMeshRenderPass::SetParams(const ForwardOpaqueMeshRenderPassParams& newParams)
    {
        IG_CHECK(newParams.MainGfxCmdList != nullptr);
        IG_CHECK(newParams.DispatchOpaqueMeshInstanceStorageBuffer != nullptr);
        IG_CHECK(newParams.RenderTarget != nullptr);
        IG_CHECK(newParams.RenderTargetView != nullptr);
        IG_CHECK(newParams.Dsv != nullptr);
        IG_CHECK(newParams.TargetViewport.width > 0.f && newParams.TargetViewport.height > 0.f);
        params = newParams;
    }

    void ForwardOpaqueMeshRenderPass::OnRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(localFrameIdx < NumFramesInFlight);
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(bindlessRootSignature != nullptr);
        IG_CHECK(dispatchMeshInstanceCmdSignature != nullptr);

        CommandList& mainGfxCmdList = *params.MainGfxCmdList;

        mainGfxCmdList.Open(pso.get());
        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        mainGfxCmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        mainGfxCmdList.SetRootSignature(*bindlessRootSignature);

        /* @todo 추후 HDR Buffer로 렌더링 할 때, 이런 중간 layout 변환이 변경 될 필요가 있음. 또는 이런 레이아웃 변환을 외부로 빼낼 필요가 있음. */
        mainGfxCmdList.AddPendingTextureBarrier(
            *params.RenderTarget,
            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
            D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
        mainGfxCmdList.FlushBarriers();

        mainGfxCmdList.ClearRenderTarget(*params.RenderTargetView);
        mainGfxCmdList.SetRenderTarget(*params.RenderTargetView, *params.Dsv);
        mainGfxCmdList.SetViewport(params.TargetViewport);
        mainGfxCmdList.SetScissorRect(params.TargetViewport);

        mainGfxCmdList.ExecuteIndirect(*dispatchMeshInstanceCmdSignature, *params.DispatchOpaqueMeshInstanceStorageBuffer);

        mainGfxCmdList.Close();
    }
}
