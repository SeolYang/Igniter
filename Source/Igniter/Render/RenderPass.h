#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    class CommandContext;
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        /* Record Commands */
        virtual void Render(const LocalFrameIndex localFrameIdx, CommandContext& cmdCtx) = 0;
    };
}    // namespace ig
