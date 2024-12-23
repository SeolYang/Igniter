#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"

namespace ig
{
    template <typename Ty>
    using RenderResource = Handle<Ty, class RenderContext>;

    template <typename Ty>
    struct FrameResource
    {
    public:
        eastl::array<Ty, NumFramesInFlight> LocalFrameResources{};
    };
}    // namespace ig