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

        virtual GpuSyncPoint Render(const LocalFrameIndex localFrameIdx) override;

        void SetTargetCanvas(ImGuiCanvas* canvasToRender = nullptr) { this->targetCanvas = canvasToRender; }
        void SetMainPipelineSyncPoint(GpuSyncPoint syncPoint) { mainPipelineSyncPoint = syncPoint; }

    private:
        RenderContext& renderContext;
        ImGuiCanvas* targetCanvas = nullptr;
        GpuSyncPoint mainPipelineSyncPoint;
    };
}    // namespace ig