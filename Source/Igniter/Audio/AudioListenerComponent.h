#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct AudioListenerComponent
    {
        Vector3 PrevPosition = Vector3::Zero;
    };

    template <>
    void OnInspector<AudioListenerComponent>(Registry* registry, const Entity entity);

    IG_META_DECLARE(AudioListenerComponent);
}
