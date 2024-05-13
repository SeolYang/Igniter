#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    class RenderContext;
    class ImGuiContext final
    {
    public:
        ImGuiContext(RenderContext& renderContext);
        ImGuiContext(const ImGuiContext&) = delete;
        ImGuiContext(ImGuiContext&&) noexcept = delete;
        ~ImGuiContext();

        ImGuiContext& operator=(const ImGuiContext&) = delete;
        ImGuiContext& operator=(ImGuiContext&&) noexcept = delete;

    private:
    };
}    // namespace ig