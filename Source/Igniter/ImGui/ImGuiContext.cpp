#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Igniter/ImGui/ImGuiContext.h"

namespace ig
{
    ImGuiContext::ImGuiContext(Window& window, RenderContext& renderContext) : renderContext(renderContext)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.Fonts->AddFontFromFileTTF("Fonts/D2Coding-ligature.ttf", 18, nullptr, io.Fonts->GetGlyphRangesKorean());
        ig::ImGuiX::SetupDefaultTheme();
        //ig::ImGuiX::SetupTransparentTheme(true, 0.75f);

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style                 = ImGui::GetStyle();
            style.WindowRounding              = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplWin32_Init(window.GetNative());

        fontSrv                   = renderContext.CreateGpuView(ig::EGpuViewType::ShaderResourceView);
        ig::GpuView*   fontSrvPtr = renderContext.Lookup(fontSrv);
        ig::GpuDevice& gpuDevice  = renderContext.GetGpuDevice();
        ImGui_ImplDX12_Init(&gpuDevice.GetNative(), ig::NumFramesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM,
                            &renderContext.GetCbvSrvUavDescriptorHeap().GetNative(), fontSrvPtr->CpuHandle, fontSrvPtr->GpuHandle);
    }

    ImGuiContext::~ImGuiContext()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        renderContext.DestroyGpuView(fontSrv);
    }

    bool ImGuiContext::HandleWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (bInputEnabled)
        {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            {
                return true;
            }
        }

        return false;
    }
} // namespace ig
