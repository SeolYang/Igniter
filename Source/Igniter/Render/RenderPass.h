#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSync.h"

namespace ig
{
    class World;
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        /* Preparation */
        virtual void PreRender(const LocalFrameIndex localFrameIdx) = 0;
        /* Record Commands */
        virtual void Render(const LocalFrameIndex localFrameIdx, World& world) = 0;
        /* Submit */
        virtual GpuSync PostRender(const LocalFrameIndex localFrameIdx) = 0;
    };
}    // namespace ig

/* A Rendering Pipeline = Composite of Render Passes */
/* A Renderer = Composite of multiple Rendering Pipelines */