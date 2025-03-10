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
            .SourcePath = "Assets/Shaders/MeshInstanceAS.hlsl"_fs,
            .Type = EShaderType::Amplification
        };
        as = MakePtr<ShaderBlob>(vsDesc);

        const ShaderCompileDesc msDesc{
            .SourcePath = "Assets/Shaders/DepthOnlyMeshInstanceMS.hlsl"_fs,
            .Type = EShaderType::Mesh
        };
        ms = MakePtr<ShaderBlob>(msDesc);

        MeshShaderPipelineStateDesc psoDesc{};
        psoDesc.Name = "ZPrePassPSO"_fs;
        psoDesc.SetAmplificationShader(*as);
        psoDesc.SetMeshShader(*ms);
        psoDesc.SetRootSignature(bindlessRootSignature);
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{CD3DX12_DEFAULT{}};
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        pso = MakePtr<PipelineState>(gpuDevice.CreateMeshShaderPipelineState(psoDesc).value());
    }

    ZPrePass::~ZPrePass() {}

    void ZPrePass::SetParams(const ZPrePassParams& newParams)
    {
        IG_CHECK(newParams.GfxCmdList != nullptr);
        IG_CHECK(newParams.DispatchMeshInstanceCmdSignature != nullptr);
        IG_CHECK(newParams.DispatchOpaqueMeshInstanceStorageBuffer != nullptr);
        IG_CHECK(newParams.Dsv != nullptr);
        IG_CHECK(newParams.MainViewport.width > 0.f && newParams.MainViewport.height > 0.f);
        params = newParams;
    }

    void ZPrePass::OnRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(pso != nullptr);

        CommandList& cmdList = *params.GfxCmdList;
        cmdList.Open(pso.get());

        auto descriptorHeaps = renderContext->GetBindlessDescriptorHeaps();
        cmdList.SetDescriptorHeaps(MakeSpan(descriptorHeaps));
        cmdList.SetRootSignature(*bindlessRootSignature);

        cmdList.ClearDepth(*params.Dsv, 0.f);
        cmdList.SetDepthOnlyTarget(*params.Dsv);
        cmdList.SetViewport(params.MainViewport);
        cmdList.SetScissorRect(params.MainViewport);
        cmdList.ExecuteIndirect(*params.DispatchMeshInstanceCmdSignature, *params.DispatchOpaqueMeshInstanceStorageBuffer);

        cmdList.Close();
    }
} // namespace ig
