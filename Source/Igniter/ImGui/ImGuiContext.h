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

    private:
        RenderContext& renderContext;
        Handle<GpuView, RenderContext> fontSrv{};

    };
}    // namespace ig