#pragma once

namespace ig
{
    enum class ELightType
    {
        Point,
    };

    struct Light
    {
        ELightType Type = ELightType::Point;
        float Radius = 1.f;
    };
} // namespace ig
