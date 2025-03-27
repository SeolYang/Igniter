#pragma once
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/MaterialComponent.h"
#include "Igniter/Component/RenderableTag.h"
#include "Igniter/Component/Archetype.h"

namespace ig
{
    using StaticMeshArchetype = Archetype<TransformComponent, StaticMeshComponent, MaterialComponent, RenderableTag>;
    IG_META_DECLARE(StaticMeshArchetype);
} // namespace ig
