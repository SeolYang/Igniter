#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    template <typename Ty>
    using RenderResource = Handle<Ty, class RenderContext>;

    template <typename Ty>
    struct LocalFrameResource
    {
    public:
        eastl::array<Ty, NumFramesInFlight> Resources;
    };
}    // namespace ig