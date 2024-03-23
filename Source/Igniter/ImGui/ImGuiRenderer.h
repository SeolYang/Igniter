#pragma once
#include <Core/Container.h>
#include <memory>

namespace ig
{
    class RenderDevice;
    class DescriptorHeap;
    class CommandContext;
} // namespace ig

namespace ig
{
    class FrameManager;
    class Window;
    class Renderer;
    class ImGuiCanvas;
    class ImGuiRenderer
    {
    public:
        ImGuiRenderer(const FrameManager& engineFrameManager, Window& window, RenderDevice& device);
        ~ImGuiRenderer();

        void Render(ImGuiCanvas& canvas, Renderer& renderer);

    private:
        const FrameManager& frameManager;
        std::unique_ptr<DescriptorHeap> descriptorHeap;
        std::vector<std::unique_ptr<CommandContext>> commandContexts;
    };
} // namespace ig