#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Swapchain.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/ImGui/ImGuiRenderer.h"

namespace ig
{
    ImGuiRenderer::ImGuiRenderer(RenderContext& renderContext) : renderContext(renderContext) {}

    GpuSync ImGuiRenderer::Render(const LocalFrameIndex localFrameIdx)
    {
        if (targetCanvas == nullptr)
        {
            return mainPipelineSyncPoint;
        }

        Swapchain& swapchain = renderContext.GetSwapchain();
        GpuTexture* backBufferPtr = renderContext.Lookup(swapchain.GetBackBuffer());
        if (backBufferPtr == nullptr)
        {
            return mainPipelineSyncPoint;
        }

        const ig::GpuView* backBufferRtvPtr = renderContext.Lookup(swapchain.GetRenderTargetView());
        if (backBufferRtvPtr == nullptr)
        {
            return mainPipelineSyncPoint;
        }

        ig::CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        auto imguiCmdCtx = renderContext.GetMainGfxCommandContextPool().Request(localFrameIdx, "ImGuiCommandContext"_fs);
        imguiCmdCtx->Begin();
        {
            imguiCmdCtx->AddPendingTextureBarrier(*backBufferPtr, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            imguiCmdCtx->FlushBarriers();

            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            targetCanvas->OnImGui();

            // @test Multiple-Viewports 기능을 위한 테스트 코드
            if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::ShowDemoWindow(nullptr);
            }

            ImGui::Render();

            imguiCmdCtx->SetRenderTarget(*backBufferRtvPtr);
            imguiCmdCtx->SetDescriptorHeap(renderContext.GetCbvSrvUavDescriptorHeap());
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &imguiCmdCtx->GetNative());

            imguiCmdCtx->AddPendingTextureBarrier(*backBufferPtr, D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            imguiCmdCtx->FlushBarriers();
        }
        imguiCmdCtx->End();

        ig::CommandContext* renderCmdCtxPtrs[] = {(ig::CommandContext*) imguiCmdCtx};
        mainGfxQueue.SyncWith(mainPipelineSyncPoint);
        mainGfxQueue.ExecuteContexts(renderCmdCtxPtrs);

        if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        return mainGfxQueue.MakeSync();
    }
}    // namespace ig