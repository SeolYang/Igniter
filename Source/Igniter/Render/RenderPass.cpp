#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    void RenderPass::Execute(const LocalFrameIndex localFrameIdx)
    {
        if (bIsActivated)
        {
            PreRender(localFrameIdx);
            Render(localFrameIdx);
            PostRender(localFrameIdx);
        }
        else
        {
            ExecuteWhenInactive(localFrameIdx);
        }
    }
}    // namespace ig