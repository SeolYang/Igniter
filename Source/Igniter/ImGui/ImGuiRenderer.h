#pragma once
#include "Igniter/Render/Renderer.h"

namespace ig
{
    class RenderContext;
    class ImGuiCanvas;

    class ImGuiRenderer
    {
    public:
        ImGuiRenderer(RenderContext& renderContext);
        ~ImGuiRenderer() = default;

        GpuSyncPoint Render(const LocalFrameIndex localFrameIdx);

        void SetTargetCanvas(ImGuiCanvas* canvasToRender = nullptr) { this->targetCanvas = canvasToRender; }
        void SetMainPipelineSyncPoint(GpuSyncPoint syncPoint) { mainPipelineSyncPoint = syncPoint; }

    private:
        RenderContext& renderContext;
        ImGuiCanvas*   targetCanvas = nullptr;
        GpuSyncPoint   mainPipelineSyncPoint;
    };
} // namespace ig
