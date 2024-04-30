#pragma once
#include <Igniter.h>
#include <D3D12/GpuView.h>

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
        ImGuiRenderer(const FrameManager& frameManager, Window& window, RenderContext& renderContext);
        ~ImGuiRenderer();

        void Render(ImGuiCanvas* canvas, Renderer& renderer);

    private:
        void SetupDefaultTheme();

    private:
        const FrameManager& frameManager;
        RenderContext& renderContext;
        Ptr<GpuViewManager> gpuViewManager;
        RenderResource<GpuView> mainSrv;

        std::vector<CommandContext> commandContexts;
    };
}    // namespace ig
