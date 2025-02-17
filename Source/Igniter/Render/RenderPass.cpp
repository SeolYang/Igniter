#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    void RenderPass::Record(const LocalFrameIndex localFrameIdx)
    {
        if (bIsActivated)
        {
            PreRecord(localFrameIdx);
            OnRecord(localFrameIdx);
            PostRecord(localFrameIdx);
        }
        else
        {
            RecordWhenInactive(localFrameIdx);
        }
    }
} // namespace ig
