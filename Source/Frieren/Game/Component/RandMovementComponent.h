#pragma once
#include "Igniter/Igniter.h"

namespace fe
{
    struct RandMovementComponent
    {
        ig::Vector3 MoveDirection{};
        ig::F32 MoveSpeed{1.f};
        ig::Vector3 Rotation{};
        ig::F32 RotateSpeed{1.f};
    };
} // namespace fe
