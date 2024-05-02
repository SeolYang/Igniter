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
    class Renderer;
    class ImGuiCanvas;
    class GpuViewManager;
    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(Window& window, RenderContext& renderContext);
        ~ImGuiRenderer();

        void Render(const LocalFrameIndex localFrameIdx, ImGuiCanvas* canvas);

    private:
        void SetupDefaultTheme();

    private:
        RenderContext& renderContext;
        Ptr<GpuViewManager> gpuViewManager;
        RenderResource<GpuView> mainSrv;

        std::vector<CommandContext> commandContexts;
    };
}    // namespace ig
