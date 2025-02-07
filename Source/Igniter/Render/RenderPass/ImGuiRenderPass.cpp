#include "Igniter/Igniter.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RenderPass/ImGuiRenderPass.h"

namespace ig
{
    ImGuiRenderPass::ImGuiRenderPass(RenderContext& renderContext)
        : renderContext(&renderContext)
    {
    }

    ImGuiRenderPass::~ImGuiRenderPass()
    {
        /* Empty! */
    }

    void ImGuiRenderPass::SetParams(const ImGuiRenderPassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.BackBuffer);
        IG_CHECK(newParams.BackBufferRtv);
        IG_CHECK(newParams.MainViewport.width > 0.f && newParams.MainViewport.height > 0.f);
        params = newParams;
    }

    void ImGuiRenderPass::OnExecute([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        IG_CHECK(renderContext != nullptr);

        ImDrawData* imGuiDrawData = ImGui::GetDrawData();

        GpuTexture* backBuffer = renderContext->Lookup(params.BackBuffer);
        IG_CHECK(backBuffer != nullptr);
        const GpuView* backBufferRtv = renderContext->Lookup(params.BackBufferRtv);
        IG_CHECK(backBufferRtv != nullptr);

        CommandList& cmdList = *params.CmdList;
        cmdList.Open();

        if (imGuiDrawData != nullptr)
        {
            cmdList.SetRenderTarget(*backBufferRtv);
            cmdList.SetDescriptorHeap(renderContext->GetCbvSrvUavDescriptorHeap());
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &cmdList.GetNative());
        }

        cmdList.AddPendingTextureBarrier(
            *backBuffer,
            D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
            D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
            D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
        cmdList.FlushBarriers();

        cmdList.Close();
    }
} // namespace ig
