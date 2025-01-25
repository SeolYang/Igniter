#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    void RenderPass::Execute(const LocalFrameIndex localFrameIdx)
    {
        if (bIsActivated)
        {
            PreExecute(localFrameIdx);
            OnExecute(localFrameIdx);
            PostExecute(localFrameIdx);
        }
        else
        {
            ExecuteWhenInactive(localFrameIdx);
        }
    }
} // namespace ig
