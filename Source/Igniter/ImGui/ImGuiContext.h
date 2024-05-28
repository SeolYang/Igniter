#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"

namespace ig
{
    class GpuView;
    class RenderContext;
    class ImGuiContext final
    {
    public:
        ImGuiContext(Window& window, RenderContext& renderContext);
        ImGuiContext(const ImGuiContext&) = delete;
        ImGuiContext(ImGuiContext&&) noexcept = delete;
        ~ImGuiContext();

        ImGuiContext& operator=(const ImGuiContext&) = delete;
        ImGuiContext& operator=(ImGuiContext&&) noexcept = delete;

        void SetInputEnabled(const bool bEnabled)
        {
            bInputEnabled = bEnabled;
            if (bEnabled)
            {
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            else
            {
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
        }

        void HandleWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        RenderContext& renderContext;
        Handle<GpuView, RenderContext> fontSrv{};
        bool bInputEnabled = true;
    };
}    // namespace ig