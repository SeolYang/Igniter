#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Swapchain.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/ImGui/ImGuiRenderer.h"

namespace ig
{
    ImGuiRenderer::ImGuiRenderer(RenderContext& renderContext) :
        renderContext(renderContext) {}

    GpuSyncPoint ImGuiRenderer::Render(const LocalFrameIndex localFrameIdx)
    {
        if (targetCanvas == nullptr)
        {
            return mainPipelineSyncPoint;
        }

        Swapchain&  swapchain     = renderContext.GetSwapchain();
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
        ig::GpuFence&     mainGfxFence = renderContext.GetMainGfxFence();
        auto              imguiCmdList  = renderContext.GetMainGfxCommandListPool().Request(localFrameIdx, "ImGuiCommandList"_fs);
        imguiCmdList->Begin();
        {
            imguiCmdList->AddPendingTextureBarrier(*backBufferPtr,
                                                  D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                                  D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                  D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            imguiCmdList->FlushBarriers();

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

            imguiCmdList->SetRenderTarget(*backBufferRtvPtr);
            imguiCmdList->SetDescriptorHeap(renderContext.GetCbvSrvUavDescriptorHeap());
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &imguiCmdList->GetNative());

            imguiCmdList->AddPendingTextureBarrier(*backBufferPtr,
                                                  D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                                                  D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                  D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            imguiCmdList->FlushBarriers();
        }
        imguiCmdList->End();

        ig::CommandList* renderCmdListPtrs[] = {(ig::CommandList*)imguiCmdList};
        mainGfxQueue.Wait(mainPipelineSyncPoint);
        mainGfxQueue.ExecuteContexts(renderCmdListPtrs);

        if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        return mainGfxQueue.MakeSyncPointWithSignal(mainGfxFence);
    }
} // namespace ig
