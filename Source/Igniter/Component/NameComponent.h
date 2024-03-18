#pragma once
#include <Core/String.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    struct NameComponent
    {
    public:
        String Name{};
    };

    IG_DECLARE_COMPONENT(NameComponent);
} // namespace ig
