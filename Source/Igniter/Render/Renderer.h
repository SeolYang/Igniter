#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSync.h"

namespace ig
{
    class World;
    class Renderer
    {
    public:
        virtual ~Renderer() = default;
        virtual void PreRender(const LocalFrameIndex localFrameIdx) = 0;
        virtual void Render(const LocalFrameIndex localFrameIdx, World& world) = 0;
        virtual GpuSync PostRender(const LocalFrameIndex localFrameIdx) = 0;
    };
}    // namespace ig
