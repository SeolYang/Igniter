#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/D3D12/RenderDevice.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/CommandContext.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/Render/Swapchain.h"
#include "Igniter/Render/Renderer.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/GpuViewManager.h"
#include "Igniter/ImGui/ImGuiRenderer.h"
#include "Igniter/ImGui/ImGuiCanvas.h"

namespace ig
{
    ImGuiRenderer::ImGuiRenderer(Window& window, RenderContext& renderContext)
        : renderContext(renderContext)
        , mainSrv(renderContext.CreateGpuView(EGpuViewType::ShaderResourceView))
        , gpuViewManager(MakePtr<GpuViewManager>(renderContext.GetRenderDevice()))
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.Fonts->AddFontFromFileTTF("Fonts/D2Coding-ligature.ttf", 18, nullptr, io.Fonts->GetGlyphRangesKorean());
        // ImGui::StyleColorsDark();
        SetupDefaultTheme();

        GpuView* mainSrvPtr = renderContext.Lookup(mainSrv);
        RenderDevice& renderDevice = renderContext.GetRenderDevice();
        ImGui_ImplWin32_Init(window.GetNative());
        ImGui_ImplDX12_Init(&renderDevice.GetNative(), NumFramesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM,
            &renderContext.GetCbvSrvUavDescriptorHeap().GetNative(), mainSrvPtr->CPUHandle, mainSrvPtr->GPUHandle);

        commandContexts.reserve(NumFramesInFlight);
        for (size_t localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            commandContexts.emplace_back(renderDevice.CreateCommandContext("ImGui Cmd Ctx", EQueueType::Graphics).value());
        }
    }

    ImGuiRenderer::~ImGuiRenderer()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        commandContexts.clear();

        renderContext.DestroyGpuView(mainSrv);
    }

    void ImGuiRenderer::Render(const LocalFrameIndex localFrameIdx, ImGuiCanvas* canvas)
    {
        /* #sy_note Backbuffer의 최종 상태가 ImGuiRenderer에 의해 결정 되는게 아니도록 수정해야함.. */
        ZoneScoped;
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (canvas != nullptr)
        {
            canvas->OnImGui();
        }
        ImGui::Render();

        CommandContext& cmdCtx = commandContexts[localFrameIdx];
        cmdCtx.Begin();
        Swapchain& swapchain = renderContext.GetSwapchain();
        GpuTexture* backBufferPtr = renderContext.Lookup(swapchain.GetBackBuffer());
        const GpuView* backBufferRtvPtr = renderContext.Lookup(swapchain.GetRenderTargetView());
        cmdCtx.SetRenderTarget(*backBufferRtvPtr);
        cmdCtx.SetDescriptorHeap(renderContext.GetCbvSrvUavDescriptorHeap());

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &cmdCtx.GetNative());

        if (canvas != nullptr)
        {
            cmdCtx.AddPendingTextureBarrier(*backBufferPtr, D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
        }
        else
        {
            cmdCtx.AddPendingTextureBarrier(*backBufferPtr, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
        }

        cmdCtx.FlushBarriers();
        cmdCtx.End();

        CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();
        CommandContext* cmdCtxPtrs[]{&cmdCtx};
        mainGfxQueue.ExecuteContexts(cmdCtxPtrs);
    }

    void ImGuiRenderer::SetupDefaultTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.WindowPadding = ImVec2(10.0f, 8.0f);
        style.WindowRounding = 5.0f;
        style.WindowBorderSize = 0.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.ChildRounding = 5.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 5.0f;
        style.PopupBorderSize = 0.0f;
        style.FramePadding = ImVec2(10.0f, 5.0f);
        style.FrameRounding = 5.0f;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(7.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(2.0f, 4.0f);
        style.CellPadding = ImVec2(7.0f, 2.0f);
        style.IndentSpacing = 1.5f;
        style.ColumnsMinSpacing = 7.0f;
        style.ScrollbarSize = 12.5f;
        style.ScrollbarRounding = 20.0f;
        style.GrabMinSize = 6.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 4.0f;
        style.TabBorderSize = 0.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style.Colors[ImGuiCol_Text] = ImVec4(0.8627451062202454f, 0.8823529481887817f, 0.8980392217636108f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6196078658103943f, 0.6196078658103943f, 0.6196078658103943f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2196078449487686f, 0.2196078449487686f, 0.2196078449487686f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.1411764770746231f, 0.1411764770746231f, 0.1411764770746231f, 1.0f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.09803921729326248f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3411764800548553f, 0.3411764800548553f, 0.3411764800548553f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.501960813999176f, 0.501960813999176f, 0.501960813999176f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.501960813999176f, 0.501960813999176f, 0.501960813999176f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3411764800548553f, 0.3411764800548553f, 0.3411764800548553f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.8235294222831726f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3532207012176514f, 0.5698376893997192f, 0.8068669438362122f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.1215686276555061f, 0.1215686276555061f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(1.0f, 1.0f, 1.0f, 0.105882354080677f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 1.0f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 0.7529411911964417f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
    }
}    // namespace ig
