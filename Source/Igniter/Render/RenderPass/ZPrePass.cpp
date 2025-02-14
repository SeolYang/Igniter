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
#include "Igniter/Render/RenderPass/ZPrePass.h"

namespace ig
{
    ZPrePass::ZPrePass(RenderContext& renderContext, RootSignature& bindlessRootSignature)
        : renderContext(&renderContext)
        , bindlessRootSignature(&bindlessRootSignature)
    {
        IG_CHECK(this->renderContext != nullptr);
        IG_CHECK(this->bindlessRootSignature != nullptr);

        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        const ShaderCompileDesc vsDesc{
            .SourcePath = "Assets/Shaders/ZPrePass.hlsl"_fs,
            .Type = EShaderType::Vertex
        };

        vs = MakePtr<ShaderBlob>(vsDesc);

        GraphicsPipelineStateDesc psoDesc;
        psoDesc.Name = "ZPrePassPSO"_fs;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetRootSignature(bindlessRootSignature);
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{CD3DX12_DEFAULT{}};
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        pso = MakePtr<PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());
    }

    void ZPrePass::SetParams(const ZPrePassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.DrawInstanceCmdSignature != nullptr);
        IG_CHECK(newParams.MainViewport.width > 0.f && newParams.MainViewport.height > 0.f);
        params = newParams;
    }

    void ZPrePass::OnExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(pso != nullptr);

        GpuBuffer* drawInstanceCmdStorageBuffer = renderContext->Lookup(params.DrawInstanceCmdStorageBuffer);
        IG_CHECK(drawInstanceCmdStorageBuffer != nullptr);
        const GpuView* dsv = renderContext->Lookup(params.Dsv);
        IG_CHECK(dsv != nullptr);

        CommandList& cmdList = *params.CmdList;
        cmdList.Open(pso.get());

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        cmdList.ClearDepth(*dsv, 0.f);
        cmdList.SetDepthOnlyTarget(*dsv);
        cmdList.SetViewport(params.MainViewport);
        cmdList.SetScissorRect(params.MainViewport);
        cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList.ExecuteIndirect(*params.DrawInstanceCmdSignature, *drawInstanceCmdStorageBuffer);

        cmdList.Close();
    }
} // namespace ig
