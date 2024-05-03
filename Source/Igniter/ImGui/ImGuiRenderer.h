#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuView.h"
#include "Igniter/Render/RenderPass.h"

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
    class ImGuiRenderer final : public RenderPass
    {
    public:
        ImGuiRenderer(Window& window, RenderContext& renderContext);
        ~ImGuiRenderer();

        void SetImGuiCanvas(ImGuiCanvas* const newCanvas) { canvas = newCanvas; }

        void PreRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) override {}
        GpuSync Render(const LocalFrameIndex localFrameIdx, const std::span<GpuSync> syncs) override;
        void PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) override {}

    private:
        void SetupDefaultTheme();

    private:
        RenderContext& renderContext;
        ImGuiCanvas* canvas = nullptr;

        RenderResource<GpuView> mainSrv;

        std::vector<CommandContext> commandContexts;
    };
}    // namespace ig
