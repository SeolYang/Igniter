#pragma once
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/Archetype.h"

namespace ig
{
    using CameraArchetype = Archetype<CameraComponent, TransformComponent>;
    IG_META_DECLARE(CameraArchetype);
} // namespace ig
