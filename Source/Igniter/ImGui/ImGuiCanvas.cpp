#include "Igniter/Igniter.h"
#include "Igniter/ImGui/ImGuiCanvas.h"

namespace ig
{
    void ImGuiCanvas::Render()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        this->OnGui();
        ImGui::Render();

        if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
} // namespace ig
