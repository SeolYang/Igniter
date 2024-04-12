#pragma once
#include <Igniter.h>
#include <D3D12/GpuView.h>

namespace ig
{
    class RenderDevice;
    class DescriptorHeap;
    class CommandContext;
    class GpuView;
    class FrameManager;
    class Window;
    class Renderer;
    class ImGuiCanvas;
    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(const FrameManager& frameManager, Window& window, RenderDevice& device);
        ~ImGuiRenderer();

        void Render(ImGuiCanvas& canvas, Renderer& renderer);

        GpuView GetReservedShaderResourceView() const;

    private:
        void SetupDefaultTheme();

    private:
        const FrameManager& frameManager;

        Ptr<DescriptorHeap> descriptorHeap;
        GpuView mainSrv;
        std::vector<GpuView> reservedSharedResourceViews;

        std::vector<Ptr<CommandContext>> commandContexts;
    };
} // namespace ig