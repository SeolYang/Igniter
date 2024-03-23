#pragma once
#include <Igniter.h>

namespace ig
{
    class GameMode
    {
    public:
        GameMode() = default;
        virtual ~GameMode() = default;
        virtual void Update(Registry& registry) = 0;
    };
} // namespace ig