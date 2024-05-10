#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuView.h"

namespace ig
{
    class RenderContext;
    class DescriptorHeap;
    class CommandContext;
    class GpuView;
    class FrameManager;
    class Window;
    class ImGuiCanvas;
    class GpuViewManager;
    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(Window& window, RenderContext& renderContext);
        ~ImGuiRenderer();

        void SetImGuiCanvas(ImGuiCanvas* const newCanvas) { canvas = newCanvas; }
        GpuSync Render(const LocalFrameIndex localFrameIdx, GpuSync appSync);

    private:
        void SetupDefaultTheme();

    private:
        RenderContext& renderContext;
        ImGuiCanvas* canvas = nullptr;

        RenderResource<GpuView> mainSrv;

        std::vector<CommandContext> commandContexts;
    };
}    // namespace ig
