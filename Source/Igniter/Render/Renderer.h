#pragma once
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual void PreRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}
        virtual GpuSyncPoint Render(const LocalFrameIndex localFrameIdx) = 0;
        virtual void PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}
    };
}    // namespace ig