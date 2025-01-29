#pragma once

namespace ig
{
    enum class ELightType : U32
    {
        Point,
    };

    constexpr U32 kMaxNumLights = 8192;
    struct Light
    {
        ELightType Type = ELightType::Point;
        float Radius = 1.f;
    };
} // namespace ig
