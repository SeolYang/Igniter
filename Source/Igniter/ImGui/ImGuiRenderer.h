#pragma once
#include "Igniter/Render/Renderer.h"

namespace ig
{
    class RenderContext;
    class ImGuiCanvas;
    class ImGuiRenderer : public Renderer
    {
    public:
        ImGuiRenderer(RenderContext& renderContext);
        virtual ~ImGuiRenderer() = default;

        virtual GpuSync Render(const LocalFrameIndex localFrameIdx) override;

        void SetTargetCanvas(ImGuiCanvas* canvasToRender = nullptr) { this->targetCanvas = canvasToRender; }

    private:
        RenderContext& renderContext;
        ImGuiCanvas* targetCanvas = nullptr;
        
    };
}    // namespace ig